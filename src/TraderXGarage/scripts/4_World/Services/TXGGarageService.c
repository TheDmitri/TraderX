// TraderXGarage - Core Service
// Server-authoritative park-in / park-out, recursive snapshot + restore, fee charging.
// Ports the business logic of TraderPlusV1/src/Garage/Helpers/GarageHelpers.c into a
// DayZular service. Also holds the client-side cache of the player's stored vehicles.

class TXGGarageService extends DayZularService
{
    // Client cache of the player's garage (synced from server)
    protected ref array<ref TXGVehicleRowVM> m_ClientVehicles;
    protected int m_ClientMaxSlots = 5;

    // Fired client-side whenever the vehicle list is replaced (UI subscribes).
    static ref ScriptInvoker Event_OnVehiclesUpdated = new ScriptInvoker();
    // Fired client-side for one-shot status messages (success / error toasts).
    static ref ScriptInvoker Event_OnNotify = new ScriptInvoker();

    void TXGGarageService()
    {
        if (GetGame().IsClient() || !GetGame().IsMultiplayer())
            m_ClientVehicles = new array<ref TXGVehicleRowVM>();
    }

    // ============================================================
    //  CLIENT CACHE
    // ============================================================

    array<ref TXGVehicleRowVM> GetClientVehicles()
    {
        if (!m_ClientVehicles)
            m_ClientVehicles = new array<ref TXGVehicleRowVM>();
        return m_ClientVehicles;
    }

    int GetClientMaxSlots()
    {
        return m_ClientMaxSlots;
    }

    void SetClientVehicles(array<ref TXGVehicleRowVM> rows, int maxSlots)
    {
        m_ClientVehicles = new array<ref TXGVehicleRowVM>();
        if (rows)
        {
            foreach (TXGVehicleRowVM r : rows)
            {
                if (r)
                    m_ClientVehicles.Insert(r);
            }
        }
        m_ClientMaxSlots = maxSlots;
        Event_OnVehiclesUpdated.Invoke();
    }

    // ============================================================
    //  SERVER: PARK IN
    // ============================================================

    // Finds a nearby vehicle, snapshots it, charges the fee, stores it and deletes the world entity.
    // Returns a human-readable status string (also used by the UI toast).
    string ParkIn(PlayerBase player, vector playerPos, out bool success)
    {
        success = false;
        if (!GetGame().IsServer() || !player || !player.GetIdentity())
            return "Server error";

        TXGSettings settings = GetTXGConfigService().GetServerSettings();
        if (!settings)
            return "Garage not configured";

        CarScript car = FindNearbyCar(playerPos, settings.parkRadius);
        if (!car)
            return "No vehicle nearby to park";

        if (IsCarOccupied(car))
            return "The vehicle still has occupants";

        string uid = player.GetIdentity().GetPlainId();
        string name = player.GetIdentity().GetName();

        TXGGarageData data = TXGGarageData.Load(uid, name);
        if (data.vehicles.Count() >= settings.maxVehicleStored)
            return "Garage is full (" + settings.maxVehicleStored.ToString() + " max)";

        if (!ChargeFee(player, settings.parkInCost, TXGConstants.FEE_LABEL_OUT))
            return "Not enough money";

        TXGVehicleData record = new TXGVehicleData();
        record.vehicleId = data.AllocateVehicleId();
        record.className = car.GetType();
        record.health = car.GetHealth("", "");
        record.fuelFraction = car.GetFluidFraction(CarFluid.FUEL);
        record.timestamp = GetUtcStamp();
        record.lockCode = 0;
        record.snapshot = SnapshotItem(car);

        data.vehicles.Insert(record);
        data.Save();

        GetGame().ObjectDelete(car);

        if (settings.enableLogs)
            GetTXGLog().LogInfo("[TXG] Park-in: " + name + " stored " + record.className + " (" + record.vehicleId + ")");

        success = true;
        return "Vehicle parked";
    }

    // ============================================================
    //  SERVER: PARK OUT
    // ============================================================

    // Spawns the stored vehicle, restores its state, charges the fee and removes it from storage.
    // npcId = the garage attendant the player interacted with; drives spot resolution.
    EntityAI ParkOut(PlayerBase player, string vehicleId, int npcId, vector playerPos, out string status)
    {
        status = "Server error";
        if (!GetGame().IsServer() || !player || !player.GetIdentity())
            return null;

        TXGSettings settings = GetTXGConfigService().GetServerSettings();
        if (!settings)
        {
            status = "Garage not configured";
            return null;
        }

        string uid = player.GetIdentity().GetPlainId();
        string name = player.GetIdentity().GetName();

        TXGGarageData data = TXGGarageData.Load(uid, name);
        TXGVehicleData record = data.FindVehicle(vehicleId);
        if (!record)
        {
            status = "Vehicle not found";
            return null;
        }

        // Resolve spawn location from the NPC the player clicked (free slot + collision check).
        vector spawnPos;
        vector spawnOri;
        bool spotOk = ResolveParkSpot(settings, npcId, playerPos, spawnPos, spawnOri);
        if (!spotOk)
        {
            status = "No free parking spot available";
            return null;
        }

        EntityAI vehicle = CreateVehicle(record, spawnPos, spawnOri);
        if (!vehicle)
        {
            status = "The parking spot is blocked";
            GetTXGLog().LogWarning("[TXG] Park-out failed to spawn " + record.className + " at " + spawnPos.ToString());
            return null;
        }

        if (!ChargeFee(player, settings.parkOutCost, TXGConstants.FEE_LABEL_OUT))
        {
            GetGame().ObjectDelete(vehicle);
            status = "Not enough money";
            return null;
        }

        data.RemoveVehicle(vehicleId);
        data.Save();

        if (settings.enableLogs)
            GetTXGLog().LogInfo("[TXG] Park-out: " + name + " retrieved " + record.className + " (" + vehicleId + ")");

        status = "Vehicle retrieved";
        return vehicle;
    }

    // Resolve the spawn spot from the NPC the player clicked — mirrors TraderX's per-trader
    // parking (vehicle is placed at the interacted NPC's slots, never a global "nearest").
    // Picks the first FREE slot (collision-checked) of that NPC. Returns false if the NPC has
    // no slots or all are blocked. npcId < 0 (standalone, no GetNpcId) falls back to nearest NPC.
    protected bool ResolveParkSpot(TXGSettings settings, int npcId, vector playerPos, out vector outPos, out vector outOri)
    {
        outPos = playerPos;
        outOri = vector.Zero;

        TXGNpcEntry entry = FindNpcEntry(settings, npcId, playerPos);
        if (!entry || !entry.parkSpots || entry.parkSpots.Count() == 0)
            return false;

        foreach (TXGParkSpot spot : entry.parkSpots)
        {
            if (!spot)
                continue;
            if (IsParkingAvailable(spot.position, spot.orientation, spot.size))
            {
                outPos = spot.position;
                outOri = spot.orientation;
                return true;
            }
        }

        return false;
    }

    protected TXGNpcEntry FindNpcEntry(TXGSettings settings, int npcId, vector playerPos)
    {
        // Exact match on the interacted NPC id (TraderX mode).
        if (npcId >= 0)
        {
            foreach (TXGNpcEntry npc : settings.npcs)
            {
                if (npc && npc.npcId == npcId)
                    return npc;
            }
        }

        // Fallback (standalone / unknown id): nearest configured NPC to the player.
        TXGNpcEntry nearest;
        float best = -1;
        foreach (TXGNpcEntry n : settings.npcs)
        {
            if (!n)
                continue;
            float d = vector.Distance(playerPos, n.position);
            if (!nearest || d < best)
            {
                best = d;
                nearest = n;
            }
        }
        return nearest;
    }

    // True if the box at (pos, ori, size) is clear. size == vector.Zero skips the test.
    bool IsParkingAvailable(vector pos, vector ori, vector size)
    {
        if (size == vector.Zero)
            return true;

        array<Object> excluded = new array<Object>();
        array<Object> nearby = new array<Object>();
        GetGame().IsBoxColliding(pos, ori, size, excluded, nearby);
        return nearby.Count() == 0;
    }

    // ============================================================
    //  SERVER: SNAPSHOT (recursive)  — port of GetGarageItemData
    // ============================================================

    TXGItemSnapshot SnapshotItem(EntityAI item)
    {
        TXGItemSnapshot snap = new TXGItemSnapshot();
        snap.item = item.GetType();

        ItemBase itemBs;
        if (Class.CastTo(itemBs, item))
        {
            if (itemBs.IsLiquidContainer())
                snap.liquidType = itemBs.GetLiquidType();
            else
                snap.liquidType = 0;

            snap.quantity = itemBs.GetQuantity();
            snap.health = itemBs.GetHealth("", "");

            if (itemBs.IsFood())
            {
                Edible_Base itemFood = Edible_Base.Cast(itemBs);
                if (itemFood)
                    snap.foodstage = itemFood.GetFoodStageType();
            }

            snap.temp = itemBs.GetTemperature();
            snap.wetness = itemBs.GetWet();

            ComponentEnergyManager em = itemBs.GetCompEM();
            if (em)
                snap.energy = em.GetEnergy();
        }

        Magazine itemMag;
        if (Class.CastTo(itemMag, item))
            snap.magAmmoCount = itemMag.GetAmmoCount();

        GameInventory inv = item.GetInventory();

        InventoryLocation loc = new InventoryLocation();
        if (inv.GetCurrentInventoryLocation(loc))
        {
            snap.slot = loc.GetSlot();
            snap.isAttachment = (loc.GetType() == InventoryLocationType.ATTACHMENT);
        }

        TXGSettings settings = GetTXGConfigService().GetServerSettings();

        CargoBase cargo = inv.GetCargo();
        if (cargo && settings && settings.saveVehicleCargo)
        {
            for (int i = 0; i < cargo.GetItemCount(); i++)
            {
                EntityAI cargoItem = cargo.GetItem(i);
                if (cargoItem)
                    snap.childrens.Insert(SnapshotItem(cargoItem));
            }
        }

        for (int a = 0; a < inv.AttachmentCount(); a++)
        {
            EntityAI att = inv.GetAttachmentFromIndex(a);
            if (att)
                snap.childrens.Insert(SnapshotItem(att));
        }

        return snap;
    }

    // ============================================================
    //  SERVER: RESTORE (recursive)  — port of RestoreCargo
    // ============================================================

    void RestoreChildren(EntityAI parent, array<ref TXGItemSnapshot> childrens)
    {
        if (!parent || !childrens)
            return;

        GameInventory inv = parent.GetInventory();
        if (!inv)
            return;

        for (int i = 0; i < childrens.Count(); i++)
        {
            TXGItemSnapshot c = childrens.Get(i);
            if (!c)
                continue;

            EntityAI created = CreateChild(parent, c);
            if (!created)
            {
                GetTXGLog().LogWarning("[TXG] Could not restore " + c.item);
                continue;
            }

            ItemBase itemBs;
            if (Class.CastTo(itemBs, created))
            {
                itemBs.SetQuantity(c.quantity);
                itemBs.SetHealth("", "", c.health);
                itemBs.SetTemperature(c.temp);
                itemBs.SetWet(c.wetness);

                if (itemBs.IsLiquidContainer())
                    itemBs.SetLiquidType(c.liquidType);

                if (itemBs.IsFood())
                {
                    Edible_Base itemFood = Edible_Base.Cast(itemBs);
                    if (itemFood)
                        itemFood.ChangeFoodStage(c.foodstage);
                }

                ComponentEnergyManager em = itemBs.GetCompEM();
                if (em)
                    em.SetEnergy(c.energy);
            }

            Magazine itemMag;
            if (Class.CastTo(itemMag, created))
                itemMag.ServerSetAmmoCount(c.magAmmoCount);

            if (c.childrens && c.childrens.Count() != 0)
                RestoreChildren(created, c.childrens);
        }
    }

    protected EntityAI CreateChild(EntityAI parent, TXGItemSnapshot c)
    {
        if (c.isAttachment)
        {
            EntityAI att = parent.GetInventory().CreateAttachmentEx(c.item, c.slot);
            if (!att)
                att = parent.GetInventory().CreateAttachment(c.item);
            return att;
        }

        return parent.GetInventory().CreateInInventory(c.item);
    }

    // ============================================================
    //  SERVER: CREATE VEHICLE  — port of CreateVehicle (roll trick)
    // ============================================================

    EntityAI CreateVehicle(TXGVehicleData record, vector position, vector orientation)
    {
        Object obj = GetGame().CreateObject(record.className, vector.Zero, false, false, true);
        if (!obj)
            return null;

        obj.SetPosition(vector.Zero);
        obj.SetOrientation(orientation);

        // Roll trick: nudge yaw and restore so physics settles on the intended orientation.
        vector roll = obj.GetOrientation();
        roll[2] = roll[2] - 1;
        obj.SetOrientation(roll);
        roll[2] = roll[2] + 1;
        obj.SetOrientation(roll);

        EntityAI ent = EntityAI.Cast(obj);
        if (!ent)
        {
            GetGame().ObjectDelete(obj);
            return null;
        }

        CarScript vehicle = CarScript.Cast(ent);
        if (!vehicle)
        {
            GetGame().ObjectDelete(obj);
            return null;
        }

        TXGSettings settings = GetTXGConfigService().GetServerSettings();

        // Restore attachments + cargo tree.
        if (record.snapshot && record.snapshot.childrens)
            RestoreChildren(vehicle, record.snapshot.childrens);

        // Health.
        if (!settings || settings.saveVehicleHealth)
            vehicle.SetHealth("", "", record.health);

        // Fluids — top everything up, override fuel with the saved fraction.
        float fuelReq = vehicle.GetFluidCapacity(CarFluid.FUEL);
        float oilReq = vehicle.GetFluidCapacity(CarFluid.OIL);
        float coolantReq = vehicle.GetFluidCapacity(CarFluid.COOLANT);
        float brakeReq = vehicle.GetFluidCapacity(CarFluid.BRAKE);

        if (!settings || settings.saveVehicleFuel)
            fuelReq = vehicle.GetFluidCapacity(CarFluid.FUEL) * record.fuelFraction;

        vehicle.Fill(CarFluid.FUEL, fuelReq);
        vehicle.Fill(CarFluid.OIL, oilReq);
        vehicle.Fill(CarFluid.COOLANT, coolantReq);
        vehicle.Fill(CarFluid.BRAKE, brakeReq);

        // Final placement + roll trick again so the vehicle rests correctly.
        obj.SetPosition(position + Vector(0, 0.3, 0));
        obj.SetOrientation(orientation);
        roll = obj.GetOrientation();
        roll[2] = roll[2] - 1;
        obj.SetOrientation(roll);
        roll[2] = roll[2] + 1;
        obj.SetOrientation(roll);
        obj.Update();

        return vehicle;
    }

    // ============================================================
    //  SERVER: vehicle discovery / occupancy
    // ============================================================

    CarScript FindNearbyCar(vector pos, float radius)
    {
        array<Object> objects = new array<Object>();
        array<CargoBase> proxy = new array<CargoBase>();
        GetGame().GetObjectsAtPosition3D(pos, radius, objects, proxy);

        CarScript best;
        float bestDist = radius + 1.0;
        foreach (Object o : objects)
        {
            CarScript car = CarScript.Cast(o);
            if (!car)
                continue;
            float d = vector.Distance(pos, car.GetPosition());
            if (d < bestDist)
            {
                bestDist = d;
                best = car;
            }
        }
        return best;
    }

    bool IsCarOccupied(CarScript car)
    {
        if (!car)
            return false;
        for (int i = 0; i < car.CrewSize(); i++)
        {
            if (car.CrewMember(i))
                return true;
        }
        return false;
    }

    // ============================================================
    //  SERVER: fees  — provider first, cash fallback
    // ============================================================

    bool ChargeFee(PlayerBase player, int amount, string label)
    {
        if (amount <= 0)
            return true;

        TXGSettings settings = GetTXGConfigService().GetServerSettings();

#ifdef TraderX
        if (!settings || settings.payWithBank)
        {
            ITraderXPaymentProvider pay = TraderXProviderRegistry.GetInstance().GetBankingProvider();
            if (pay && pay.IsAvailable(player))
                return pay.DeductLabeled(player, amount, label, "Garage");
        }
#endif

        return RemoveCashFromPlayer(player, amount, settings);
    }

    protected bool RemoveCashFromPlayer(PlayerBase player, int amount, TXGSettings settings)
    {
        if (!settings)
            return false;

        int remaining = amount;

        array<EntityAI> items = new array<EntityAI>();
        player.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, items);

        array<EntityAI> toRemove = new array<EntityAI>();
        int collected = 0;
        foreach (EntityAI item : items)
        {
            if (collected >= amount)
                break;
            int value = settings.GetCurrencyValue(item.GetType());
            if (value > 0)
            {
                toRemove.Insert(item);
                collected = collected + value;
            }
        }

        if (collected < amount)
            return false;

        foreach (EntityAI rem : toRemove)
        {
            GetGame().ObjectDelete(rem);
        }
        return true;
    }

    // ============================================================
    //  SERVER: build the client list + health helpers
    // ============================================================

    array<ref TXGVehicleRowVM> BuildVehicleList(TXGGarageData data)
    {
        array<ref TXGVehicleRowVM> rows = new array<ref TXGVehicleRowVM>();
        if (!data)
            return rows;

        foreach (TXGVehicleData v : data.vehicles)
        {
            if (!v)
                continue;
            TXGVehicleRowVM row = new TXGVehicleRowVM();
            row.vehicleId = v.vehicleId;
            row.className = v.className;
            row.displayName = GetDisplayName(v.className);
            row.healthLevel = ConvertHealthToHealthLevel(v.className, v.health);
            row.healthText = GetHealthLevelText(row.healthLevel);
            rows.Insert(row);
        }
        return rows;
    }

    static string GetDisplayName(string className)
    {
        string display;
        if (GetGame().ConfigGetText("CfgVehicles " + className + " displayName", display) && display != "")
            return display;
        return className;
    }

    static int ConvertHealthToHealthLevel(string className, float currentHealth)
    {
        if (currentHealth == -1)
            return TXGConstants.HEALTH_PRISTINE;

        float maxHealth = MiscGameplayFunctions.GetTypeMaxGlobalHealth(className);
        if (maxHealth == 0)
            return TXGConstants.HEALTH_PRISTINE;

        float coeff = currentHealth / maxHealth;
        if (coeff > 0.7) return TXGConstants.HEALTH_PRISTINE;
        if (coeff > 0.5) return TXGConstants.HEALTH_WORN;
        if (coeff > 0.3) return TXGConstants.HEALTH_DAMAGED;
        if (coeff > 0.0) return TXGConstants.HEALTH_BADLY_DAMAGED;
        return TXGConstants.HEALTH_RUINED;
    }

    static string GetHealthLevelText(int level)
    {
        if (level == TXGConstants.HEALTH_PRISTINE)      return "Pristine";
        if (level == TXGConstants.HEALTH_WORN)          return "Worn";
        if (level == TXGConstants.HEALTH_DAMAGED)       return "Damaged";
        if (level == TXGConstants.HEALTH_BADLY_DAMAGED) return "Badly Damaged";
        return "Ruined";
    }

    // YYYYMMDDhhmmss UTC stamp as int-ish string-safe int (fits in int up to ~2038 only for date part).
    static int GetUtcStamp()
    {
        int year, month, day, hour, minute, second;
        GetYearMonthDayUTC(year, month, day);
        GetHourMinuteSecondUTC(hour, minute, second);
        // Pack MMDDhhmmss (drop century) so it stays within int range.
        return (((((month * 100 + day) * 100 + hour) * 100) + minute) * 100) + second;
    }

    // ============================================================
    //  SERVER: programmatic API (used by TXGGarageProvider — no fees)
    // ============================================================

    bool StoreVehicleEntity(string uid, EntityAI vehicle)
    {
        if (!GetGame().IsServer() || uid == "" || !vehicle)
            return false;

        TXGSettings settings = GetTXGConfigService().GetServerSettings();
        if (settings && GetStoredCount(uid) >= settings.maxVehicleStored)
            return false;

        TXGGarageData data = TXGGarageData.Load(uid, "");

        TXGVehicleData record = new TXGVehicleData();
        record.vehicleId = data.AllocateVehicleId();
        record.className = vehicle.GetType();
        record.health = vehicle.GetHealth("", "");
        record.timestamp = GetUtcStamp();

        CarScript car = CarScript.Cast(vehicle);
        if (car)
            record.fuelFraction = car.GetFluidFraction(CarFluid.FUEL);

        record.snapshot = SnapshotItem(vehicle);

        data.vehicles.Insert(record);
        data.Save();

        GetGame().ObjectDelete(vehicle);
        return true;
    }

    bool StoreVehicleByType(string uid, string className)
    {
        if (!GetGame().IsServer() || uid == "" || className == "")
            return false;

        TXGSettings settings = GetTXGConfigService().GetServerSettings();
        if (settings && GetStoredCount(uid) >= settings.maxVehicleStored)
            return false;

        TXGGarageData data = TXGGarageData.Load(uid, "");

        TXGVehicleData record = new TXGVehicleData();
        record.vehicleId = data.AllocateVehicleId();
        record.className = className;
        record.health = MiscGameplayFunctions.GetTypeMaxGlobalHealth(className);
        record.fuelFraction = 0;
        record.timestamp = GetUtcStamp();
        record.snapshot = new TXGItemSnapshot();
        record.snapshot.item = className;

        data.vehicles.Insert(record);
        data.Save();
        return true;
    }

    bool RemoveVehicleById(string uid, string vehicleId)
    {
        if (!GetGame().IsServer() || uid == "")
            return false;

        TXGGarageData data = TXGGarageData.Load(uid, "");
        if (!data.FindVehicle(vehicleId))
            return false;

        data.RemoveVehicle(vehicleId);
        data.Save();
        return true;
    }

    EntityAI RetrieveVehicleById(string uid, string vehicleId, vector pos, vector orientation)
    {
        if (!GetGame().IsServer() || uid == "")
            return null;

        TXGGarageData data = TXGGarageData.Load(uid, "");
        TXGVehicleData record = data.FindVehicle(vehicleId);
        if (!record)
            return null;

        EntityAI vehicle = CreateVehicle(record, pos, orientation);
        if (!vehicle)
            return null;

        data.RemoveVehicle(vehicleId);
        data.Save();
        return vehicle;
    }

    int GetStoredCount(string uid)
    {
        if (uid == "")
            return 0;
        TXGGarageData data = TXGGarageData.Load(uid, "");
        return data.vehicles.Count();
    }

    int GetMaxSlots()
    {
        TXGSettings settings = GetTXGConfigService().GetServerSettings();
        if (settings)
            return settings.maxVehicleStored;
        return 5;
    }

    bool HasFreeSlot(string uid)
    {
        return GetStoredCount(uid) < GetMaxSlots();
    }
}

// Global accessor
static ref TXGGarageService g_TXGGarageService;

TXGGarageService GetTXGGarageService()
{
    if (!g_TXGGarageService)
        g_TXGGarageService = TXGGarageService.Cast(DZlrServiceRegistry.GetInstance().GetService(TXGGarageService));
    return g_TXGGarageService;
}
