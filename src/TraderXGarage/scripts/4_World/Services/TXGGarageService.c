// TraderXGarage - Core Service
// Server-authoritative park-in / park-out, recursive snapshot + restore, fee charging.
// Ports the business logic of TraderPlusV1/src/Garage/Helpers/GarageHelpers.c into a
// DayZular service. Also holds the client-side cache of the player's stored vehicles.

class TXGGarageService extends DayZularService
{
    // Client cache of the player's garage (synced from server)
    protected ref array<ref TXGVehicleRowVM> m_ClientVehicles;
    protected int m_ClientMaxSlots = 5;
    protected ref TXGVehicleRowVM m_ClientNearby;

    // Fired client-side whenever the vehicle list is replaced (UI subscribes).
    static ref ScriptInvoker Event_OnVehiclesUpdated = new ScriptInvoker();
    // Fired client-side for one-shot status messages (success / error toasts).
    static ref ScriptInvoker Event_OnNotify = new ScriptInvoker();

    void TXGGarageService()
    {
        if (GetGame().IsClient() || !GetGame().IsMultiplayer())
            m_ClientVehicles = new array<ref TXGVehicleRowVM>();

        if (GetGame().IsServer() || !GetGame().IsMultiplayer())
            GetDayZularEvents().OnMissionStart.Insert(OnMissionStart);
    }

    void ~TXGGarageService()
    {
        if (GetDayZularEvents() && GetDayZularEvents().OnMissionStart)
            GetDayZularEvents().OnMissionStart.Remove(OnMissionStart);
    }

    protected void OnMissionStart()
    {
        if (!GetGame().IsServer())
            return;
        ScanDeadClassesAtBoot();
    }

    static bool IsEntityClassDefined(string className)
    {
        if (className == "")
            return false;
        if (GetGame().ConfigIsExisting("CfgVehicles " + className))
            return true;
        if (GetGame().ConfigIsExisting("CfgMagazines " + className))
            return true;
        if (GetGame().ConfigIsExisting("CfgWeapons " + className))
            return true;
        return false;
    }

    protected void CollectDeadItemClasses(array<ref TXGItemSnapshot> nodes, TStringArray deadItems)
    {
        if (!nodes)
            return;
        foreach (TXGItemSnapshot n : nodes)
        {
            if (!n || n.item == "")
                continue;
            if (!IsEntityClassDefined(n.item))
            {
                if (deadItems.Find(n.item) == -1)
                    deadItems.Insert(n.item);
            }
            if (n.childrens && n.childrens.Count() > 0)
                CollectDeadItemClasses(n.childrens, deadItems);
        }
    }

    protected void ScanDeadClassesAtBoot()
    {
        TXGPlayerIndex idx = TXGPlayerIndex.Get();
        if (!idx || !idx.entries)
            return;

        TStringArray deadVehicles = new TStringArray();
        TStringArray deadItems = new TStringArray();

        foreach (TXGPlayerIndexEntry entry : idx.entries)
        {
            if (!entry || entry.uid == "")
                continue;
            string path = TXGGarageData.GetFilePath(entry.uid);
            if (!FileExist(path))
                continue;

            TXGGarageData data = TXGGarageData.Load(entry.uid, entry.name);
            if (!data || !data.vehicles)
                continue;

            foreach (TXGVehicleData v : data.vehicles)
            {
                if (!v)
                    continue;
                ShareIndexAdd(entry.uid, v.sharedWith);
                if (!IsEntityClassDefined(v.className))
                {
                    if (deadVehicles.Find(v.className) == -1)
                        deadVehicles.Insert(v.className);
                    GetTXGLog().LogWarning("[TXG DEADCLASS] Missing vehicle class '" + v.className + "' in garage of " + entry.uid + " (slot " + v.vehicleId + ")");
                }
                if (v.snapshot && v.snapshot.childrens)
                    CollectDeadItemClasses(v.snapshot.childrens, deadItems);
            }
        }

        if (deadVehicles.Count() == 0 && deadItems.Count() == 0)
        {
            if (FileExist(TXGConstants.DEAD_CLASSES_FILE))
                DeleteFile(TXGConstants.DEAD_CLASSES_FILE);
            GetTXGLog().LogInfo("[TXG] Dead-class boot check: clean (" + idx.entries.Count().ToString() + " indexed player garage(s))");
            return;
        }

        FileHandle fh = OpenFile(TXGConstants.DEAD_CLASSES_FILE, FileMode.WRITE);
        if (fh != 0)
        {
            foreach (string itemCls : deadItems)
            {
                FPrintln(fh, itemCls);
            }
            foreach (string vehCls : deadVehicles)
            {
                FPrintln(fh, "# " + vehCls);
            }
            CloseFile(fh);
        }

        GetTXGLog().LogWarning("[TXG PROBLEM] Dead classes in stored garages: " + deadItems.Count().ToString() + " item class(es), " + deadVehicles.Count().ToString() + " vehicle class(es). Written to " + TXGConstants.DEAD_CLASSES_FILE);
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

    protected ref array<ref TXGSharePlayerVM> m_ClientPeople = new array<ref TXGSharePlayerVM>();
    protected int m_ClientBank = -1;
    protected int m_ClientCash = 0;
    protected int m_ClientNowUnix = 0;

    void SetClientExtras(array<ref TXGSharePlayerVM> people, int bank, int cash, int nowUnix)
    {
        m_ClientPeople = new array<ref TXGSharePlayerVM>();
        if (people)
        {
            foreach (TXGSharePlayerVM p : people)
            {
                if (p)
                    m_ClientPeople.Insert(p);
            }
        }
        m_ClientBank = bank;
        m_ClientCash = cash;
        m_ClientNowUnix = nowUnix;
    }

    array<ref TXGSharePlayerVM> GetClientPeople() { return m_ClientPeople; }
    int GetClientBank()    { return m_ClientBank; }
    int GetClientCash()    { return m_ClientCash; }
    int GetClientNowUnix() { return m_ClientNowUnix; }

    TXGVehicleRowVM GetClientNearby()
    {
        return m_ClientNearby;
    }

    void SetClientVehicles(array<ref TXGVehicleRowVM> rows, int maxSlots, array<ref TXGVehicleRowVM> nearby = null)
    {
        m_ClientNearby = null;
        if (nearby && nearby.Count() > 0)
            m_ClientNearby = nearby[0];
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
    // Everything that decides whether the vehicle next to the player can be parked, without side
    // effects. Shared by ParkIn and by the garage list (the UI shows the nearby vehicle + reason).
    // Returns "" when parking is allowed. vehicle is set whenever one is in range, even if refused.
    string ValidateParkIn(PlayerBase player, vector playerPos, out Transport vehicle, out string ownerUid, out bool sharedGuest)
    {
        vehicle = null;
        ownerUid = "";
        sharedGuest = false;
        if (!GetGame().IsServer() || !player || !player.GetIdentity())
            return "#txg_err_server";

        TXGSettings settings = GetTXGConfigService().GetServerSettings();
        if (!settings)
            return "#txg_err_not_configured";

        if (!FindGarageNear(settings, playerPos, -1))
            return "#txg_err_not_at_garage";

        vehicle = FindNearbyTransport(playerPos, settings.parkRadius);
        if (!vehicle)
            return TXGText.Msg("#txg_err_no_vehicle_within", Math.Round(settings.parkRadius).ToString());

        if (IsTransportOccupied(vehicle))
            return "#txg_err_occupied";

        // A wreck stays a wreck: storing it would let the garage service rebuild it (and an
        // insurance-claimed wreck would come back as a second vehicle).
        if (vehicle.IsRuined())
            return "#txg_err_destroyed";

        string stampPolicy;
        string stampOwner;
        int stampState;
        int stampNoResale;
        TXGVehicleStampHelper.ReadStamp(vehicle, stampPolicy, stampOwner, stampState, stampNoResale);
        if (stampState == TXGConstants.STAMP_CLAIMED)
            return "#txg_err_already_replaced";

        string uid = player.GetIdentity().GetPlainId();

        // A player the owner shared the vehicle with returns it to the OWNER's garage.
        ownerUid = uid;
        if (stampOwner != "" && stampOwner != uid && TXGVehicleStampHelper.IsSharedWith(vehicle, uid))
        {
            ownerUid = stampOwner;
            sharedGuest = true;
        }

        string lockError = "";
        if (!CheckCarLockCanParkIn(vehicle, uid, settings, sharedGuest, lockError))
            return lockError;

        TXGGarageData data = TXGGarageData.Load(ownerUid, "");
        if (CountSlotUsage(data, settings) >= settings.maxVehicleStored)
            return TXGText.Msg("#txg_err_full", settings.maxVehicleStored.ToString());

        return "";
    }

    string ParkIn(PlayerBase player, vector playerPos, out bool success)
    {
        success = false;
        Transport vehicle;
        string ownerUid;
        bool sharedGuest;
        string refusal = ValidateParkIn(player, playerPos, vehicle, ownerUid, sharedGuest);
        if (refusal != "")
            return refusal;

        TXGSettings settings = GetTXGConfigService().GetServerSettings();
        string name = player.GetIdentity().GetName();
        string ownerName = name;
        if (sharedGuest)
            ownerName = "";

        if (!ChargeFee(player, settings.parkInCost, TXGConstants.FEE_LABEL_IN))
            return "#txg_err_no_money";

        TXGGarageData data = TXGGarageData.Load(ownerUid, ownerName);
        TXGVehicleData record = CaptureVehicleRecord(data, vehicle, ownerUid, settings);
        if (sharedGuest)
            ShareIndexAdd(ownerUid, record.sharedWith);

        data.vehicles.Insert(record);
        data.Save();

        TXGVehicleStampHelper.SetGarageRemoval(vehicle, true);
        GetGame().ObjectDelete(vehicle);

        if (settings.enableLogs)
            GetTXGLog().LogInfo("[TXG] Park-in: " + name + " stored " + record.className + " (" + record.vehicleId + ")");

        success = true;
        return "#txg_msg_parked";
    }

    // The vehicle next to the player as a list row (isNearby), with the reason it can't be parked.
    // Null when the player is not at a garage or no vehicle is in range.
    TXGVehicleRowVM BuildNearbyRow(PlayerBase player)
    {
        Transport vehicle;
        string ownerUid;
        bool sharedGuest;
        string refusal = ValidateParkIn(player, player.GetPosition(), vehicle, ownerUid, sharedGuest);
        if (!vehicle)
            return null;

        TXGSettings settings = GetTXGConfigService().GetServerSettings();
        TXGVehicleData snapshot = CaptureVehicleRecordPreview(vehicle);

        TXGVehicleRowVM row = BuildRowVM(snapshot, ownerUid, false, settings, null, 0);
        row.vehicleId = TXGConstants.NEARBY_VEHICLE_ID;
        row.isNearby = true;
        row.parkInBlockedReason = refusal;
        row.repairQuote = 0;
        row.insurancePremiumQuote = 0;
        row.insuranceDeductibleQuote = 0;
        return row;
    }

    // Read-only capture of a live vehicle for display (no id allocation, no policy side effects).
    protected TXGVehicleData CaptureVehicleRecordPreview(Transport vehicle)
    {
        TXGVehicleData record = new TXGVehicleData();
        record.className = vehicle.GetType();
        record.health = vehicle.GetHealth("", "");
        record.fluidsSaved = true;
        CaptureDamageZones(vehicle, record.zones);
        CaptureFluids(vehicle, record);
#ifdef TraderX
        ITraderXCarLockProvider lockProv = TraderXProviderRegistry.GetInstance().GetCarLockProvider();
        if (lockProv && lockProv.HasCarLock(vehicle))
            record.hasCarLock = true;
#endif
        record.snapshot = SnapshotItem(vehicle);
        return record;
    }

    int CountSlotUsage(TXGGarageData data, TXGSettings settings)
    {
        if (!data || !data.vehicles)
            return 0;
        bool holdCounts = false;
        if (settings && settings.insurance)
            holdCounts = settings.insurance.holdCountsAgainstSlots;

        int cnt = 0;
        foreach (TXGVehicleData v : data.vehicles)
        {
            if (!v)
                continue;
            if (!holdCounts && v.holdReason == TXGConstants.HOLD_INSURANCE)
                continue;
            cnt = cnt + 1;
        }
        return cnt;
    }

    protected bool CheckCarLockCanParkIn(EntityAI vehicle, string uid, TXGSettings settings, bool sharedGuest, out string errorMsg)
    {
        errorMsg = "";
#ifdef TraderX
        ITraderXCarLockProvider lockProv = TraderXProviderRegistry.GetInstance().GetCarLockProvider();
        if (lockProv)
        {
            bool hasLock = lockProv.HasCarLock(vehicle);
            if (settings && settings.vehicleMustHaveLock && !hasLock)
            {
                errorMsg = "#txg_err_need_carlock";
                return false;
            }
            if (hasLock && !lockProv.IsOwner(vehicle, uid) && !(sharedGuest && !lockProv.IsLocked(vehicle)))
            {
                if (lockProv.IsLocked(vehicle))
                    errorMsg = "#txg_err_locked_other";
                else
                    errorMsg = "#txg_err_not_owner";
                return false;
            }
        }
#endif
        return true;
    }

    // Vehicle receipts (TXGReceiptService): same capture and lock rules as the garage
    TXGVehicleData CaptureReceiptRecord(EntityAI vehicle, string uid, string receiptId)
    {
        TXGVehicleData record = CaptureVehicleRecord(new TXGGarageData(), vehicle, uid, GetTXGConfigService().GetServerSettings());
        record.vehicleId = receiptId;
        return record;
    }

    bool CanPackReceipt(EntityAI vehicle, string uid, out string errorMsg)
    {
        return CheckCarLockCanParkIn(vehicle, uid, GetTXGConfigService().GetServerSettings(), false, errorMsg);
    }

    protected TXGVehicleData CaptureVehicleRecord(TXGGarageData data, EntityAI vehicle, string uid, TXGSettings settings)
    {
        TXGVehicleData record = new TXGVehicleData();
        record.vehicleId = data.AllocateVehicleId();
        record.className = vehicle.GetType();
        record.health = vehicle.GetHealth("", "");
        record.timestamp = GetUtcStamp();
        record.fluidsSaved = true;

        Transport transport = Transport.Cast(vehicle);
        if (transport)
        {
            CaptureDamageZones(transport, record.zones);
            CaptureFluids(transport, record);
        }

        string stampPolicyId;
        string stampOwnerUid;
        int stampState;
        int stampNoResale;
        if (TXGVehicleStampHelper.ReadStamp(vehicle, stampPolicyId, stampOwnerUid, stampState, stampNoResale))
        {
            record.noResaleUntil = stampNoResale;
            if (stampOwnerUid == uid)
                TXGVehicleStampHelper.GetSharedWith(vehicle, record.sharedWith);
            if (stampPolicyId != "")
            {
                if (stampOwnerUid != "" && stampOwnerUid != uid)
                {
                    TXGInsuranceService insSvc = GetTXGInsuranceService();
                    if (insSvc)
                        insSvc.VoidPolicy(stampOwnerUid, stampPolicyId, "OWNER_CHANGED");
                }
                else
                {
                    record.policyId = stampPolicyId;
                }
            }
        }

#ifdef TraderX
        if (!settings || settings.saveVehicleLock)
        {
            ITraderXCarLockProvider lockProv = TraderXProviderRegistry.GetInstance().GetCarLockProvider();
            if (lockProv && lockProv.HasCarLock(vehicle))
            {
                record.hasCarLock = true;
                record.lockCode = lockProv.GetPassword(vehicle);
                if (uid != "" && lockProv.IsOwner(vehicle, uid))
                    record.lockOwnerUid = uid;
                else
                    record.lockOwnerUid = lockProv.GetOwner(vehicle);
                if (record.lockOwnerUid == "")
                    record.lockOwnerUid = uid;
            }
        }
#endif

        record.snapshot = SnapshotItem(vehicle);
        return record;
    }

    protected void CaptureDamageZones(Transport vehicle, array<ref TXGZoneHealth> outZones)
    {
        if (!vehicle || !outZones)
            return;
        outZones.Clear();
        TStringArray zoneNames = new TStringArray();
        vehicle.GetDamageZones(zoneNames);
        foreach (string zName : zoneNames)
        {
            if (zName == "")
                continue;
            outZones.Insert(new TXGZoneHealth(zName, vehicle.GetHealth01(zName, "")));
        }
    }

    protected void CaptureFluids(Transport vehicle, TXGVehicleData record)
    {
        if (!vehicle || !record)
            return;

        CarScript car = CarScript.Cast(vehicle);
        if (car)
        {
            record.fuelFraction = car.GetFluidFraction(CarFluid.FUEL);
            record.oilFraction = car.GetFluidFraction(CarFluid.OIL);
            record.coolantFraction = car.GetFluidFraction(CarFluid.COOLANT);
            record.brakeFraction = car.GetFluidFraction(CarFluid.BRAKE);
            return;
        }

        BoatScript boat = BoatScript.Cast(vehicle);
        if (boat)
        {
            record.fuelFraction = boat.GetFluidFraction(BoatFluid.FUEL);
            return;
        }

        MotorbikeScript bike = MotorbikeScript.Cast(vehicle);
        if (bike)
        {
            record.fuelFraction = bike.GetFluidFraction(ETransportFluid.FUEL);
            return;
        }
    }

    // ============================================================
    //  SERVER: PARK OUT
    // ============================================================

    // Spawns the stored vehicle, restores its state, charges the fee and removes it from storage.
    // npcId = the garage attendant the player interacted with; drives spot resolution.
    EntityAI ParkOut(PlayerBase player, string vehicleId, int npcId, vector playerPos, out string status)
    {
        status = "#txg_err_server";
        if (!GetGame().IsServer() || !player || !player.GetIdentity())
            return null;

        TXGSettings settings = GetTXGConfigService().GetServerSettings();
        if (!settings)
        {
            status = "#txg_err_not_configured";
            return null;
        }

        string uid = player.GetIdentity().GetPlainId();
        string name = player.GetIdentity().GetName();

        TXGNpcEntry garage = FindGarageNear(settings, playerPos, npcId);
        if (!garage)
        {
            status = "#txg_err_not_at_garage";
            return null;
        }

        string blockedZone = "";
        if (IsInNoRetrieveZone(playerPos, settings, blockedZone))
        {
            status = TXGText.Msg("#txg_err_zone", blockedZone);
            return null;
        }

        TXGGarageData data = TXGGarageData.Load(uid, name);
        TXGVehicleData record = data.FindVehicle(vehicleId);
        if (!record)
        {
            TXGGarageData sharedOwnerData;
            if (FindSharedVehicle(uid, vehicleId, sharedOwnerData, record))
                data = sharedOwnerData;
        }
        if (!record)
        {
            status = "#txg_err_vehicle_not_found";
            return null;
        }

        if (!IsEntityClassDefined(record.className))
        {
            status = TXGText.Msg("#txg_err_mod_missing", record.className);
            GetTXGLog().LogWarning("[TXG] Refused park-out of dead vehicle class " + record.className + " for " + name);
            return null;
        }

        // Resolve spawn location from the NPC the player clicked (free slot + collision check).
        vector spawnPos;
        vector spawnOri;
        bool spotOk = ResolveParkSpot(garage, spawnPos, spawnOri);
        if (!spotOk)
        {
            status = "#txg_err_no_spot";
            return null;
        }

        int feeToCharge = settings.parkOutCost;
        string feeLabel = TXGConstants.FEE_LABEL_OUT;
        if (record.holdReason != "" && record.releaseFee > 0)
        {
            feeToCharge = record.releaseFee;
            if (record.holdReason == TXGConstants.HOLD_INSURANCE)
                feeLabel = TXGConstants.FEE_LABEL_INSURANCE;
        }

        int skippedItems = 0;
        EntityAI vehicle = CreateVehicle(record, spawnPos, spawnOri, data.ownerUID, skippedItems);
        if (!vehicle)
        {
            status = "#txg_err_spot_blocked";
            GetTXGLog().LogWarning("[TXG] Park-out failed to spawn " + record.className + " at " + spawnPos.ToString());
            return null;
        }

        if (!ChargeFee(player, feeToCharge, feeLabel))
        {
            GetGame().ObjectDelete(vehicle);
            status = TXGText.Msg("#txg_err_no_money_required", feeToCharge.ToString());
            return null;
        }

        data.RemoveVehicle(vehicleId);
        data.Save();

        if (settings.enableLogs)
            GetTXGLog().LogInfo("[TXG] Park-out: " + name + " retrieved " + record.className + " (" + vehicleId + "), skippedItems=" + skippedItems.ToString());

        if (skippedItems > 0)
            status = TXGText.Msg("#txg_msg_retrieved_skipped", skippedItems.ToString());
        else
            status = "#txg_msg_retrieved";
        return vehicle;
    }

    // First FREE slot (collision-checked) of the garage the player is standing at.
    protected bool ResolveParkSpot(TXGNpcEntry entry, out vector outPos, out vector outOri)
    {
        outPos = vector.Zero;
        outOri = vector.Zero;
        if (!entry || !entry.parkSpots)
            return false;

        foreach (TXGParkSpot spot : entry.parkSpots)
        {
            if (spot && IsParkingAvailable(spot.position, spot.orientation, spot.size))
            {
                outPos = spot.position;
                outOri = spot.orientation;
                return true;
            }
        }
        return false;
    }

    // The garage the player is standing at (server truth — RPCs carry no trusted position).
    // npcId >= 0 must be that garage; npcId < 0 (standalone class match) takes the nearest one.
    TXGNpcEntry FindGarageNear(TXGSettings settings, vector playerPos, int npcId)
    {
        if (!settings || !settings.npcs)
            return null;

        TXGNpcEntry nearest;
        float best = TXGConstants.GARAGE_INTERACT_DISTANCE;
        foreach (TXGNpcEntry npc : settings.npcs)
        {
            if (!npc)
                continue;
            if (npcId >= 0 && npc.npcId != npcId)
                continue;
            float d = vector.Distance(playerPos, npc.position);
            if (d <= best)
            {
                best = d;
                nearest = npc;
            }
        }
        return nearest;
    }

    bool IsAtGarage(PlayerBase player)
    {
        if (!player)
            return false;
        return FindGarageNear(GetTXGConfigService().GetServerSettings(), player.GetPosition(), -1) != null;
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

    void RestoreChildren(EntityAI parent, array<ref TXGItemSnapshot> childrens, out int skippedCount)
    {
        if (!parent || !childrens)
            return;

        GameInventory inv = parent.GetInventory();
        if (!inv)
            return;

        for (int i = 0; i < childrens.Count(); i++)
        {
            TXGItemSnapshot c = childrens.Get(i);
            if (!c || c.item == "")
                continue;

            if (!IsEntityClassDefined(c.item))
            {
                skippedCount = skippedCount + 1;
                GetTXGLog().LogWarning("[TXG] Dead class skipped on restore: " + c.item);
                continue;
            }

            EntityAI created = CreateChild(parent, c);
            if (!created)
            {
                skippedCount = skippedCount + 1;
                GetTXGLog().LogWarning("[TXG] Could not restore " + c.item);
                continue;
            }

            ItemBase itemBs;
            if (Class.CastTo(itemBs, created))
            {
                // -1 = unknown (TraderPlus receipt parts): keep what the item spawned with
                if (c.quantity >= 0)
                    itemBs.SetQuantity(c.quantity);
                if (c.health >= 0)
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
                if (em && c.energy >= 0)
                    em.SetEnergy(c.energy);
            }

            Magazine itemMag;
            if (Class.CastTo(itemMag, created))
                itemMag.ServerSetAmmoCount(c.magAmmoCount);

            if (c.childrens && c.childrens.Count() != 0)
            {
                int childSkipped = 0;
                RestoreChildren(created, c.childrens, childSkipped);
                skippedCount = skippedCount + childSkipped;
            }
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

    EntityAI CreateVehicle(TXGVehicleData record, vector position, vector orientation, string ownerUid = "", out int skippedItems = 0)
    {
        skippedItems = 0;
        if (!record || !IsEntityClassDefined(record.className))
            return null;

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

        Transport vehicle = Transport.Cast(ent);
        if (!vehicle)
        {
            GetGame().ObjectDelete(obj);
            return null;
        }

        TXGSettings settings = GetTXGConfigService().GetServerSettings();

        // Restore attachments + cargo tree.
        if (record.snapshot && record.snapshot.childrens)
            RestoreChildren(vehicle, record.snapshot.childrens, skippedItems);

        // Health + Damage Zones. TraderX core spawns cars with a 30 s god mode (SetAllowDamage(false)),
        // which makes SetHealth a no-op: lift it while restoring, or a damaged car comes back pristine.
        if (!settings || settings.saveVehicleHealth)
        {
            vehicle.SetAllowDamage(true);
            if (record.health >= 0)
                vehicle.SetHealth("", "", record.health);
            if (record.zones && record.zones.Count() > 0)
            {
                foreach (TXGZoneHealth zh : record.zones)
                {
                    if (zh && zh.zone != "")
                        vehicle.SetHealth01(zh.zone, "", zh.health01);
                }
            }
#ifdef TraderX
            vehicle.SetAllowDamage(false);   // core's spawn timer re-enables damage
#endif
        }

        // Fluids — restore exact saved fractions across CarScript / BoatScript / MotorbikeScript.
        ApplyFluids(vehicle, record, settings);

        // CarLock round-trip.
#ifdef TraderX
        if ((!settings || settings.saveVehicleLock) && record.hasCarLock)
        {
            string lockOwner = record.lockOwnerUid;
            if (lockOwner == "")
                lockOwner = ownerUid;
            ITraderXCarLockProvider lockProv = TraderXProviderRegistry.GetInstance().GetCarLockProvider();
            if (lockProv && lockOwner != "")
                lockProv.RestoreCarLock(vehicle, lockOwner, record.lockCode, true);
        }
#endif

        // Insurance / Ownership CF ModStorage stamp round-trip.
        string stampOwner = record.lockOwnerUid;
        if (stampOwner == "")
            stampOwner = ownerUid;
        int stampState = TXGConstants.STAMP_NONE;
        if (record.policyId != "")
            stampState = TXGConstants.STAMP_INSURED;
        else if (settings && settings.insurance && settings.insurance.enabled && settings.insurance.recoveryMode == "ALL_OWNED")
            stampState = TXGConstants.STAMP_INSURED;
        TXGVehicleStampHelper.ApplyStamp(vehicle, record.policyId, stampOwner, stampState, record.noResaleUntil);
        TXGVehicleStampHelper.SetSharedWith(vehicle, record.sharedWith);

        // Final placement + roll trick again so the vehicle rests correctly.
        obj.SetPosition(position + Vector(0, 0.3, 0));
        obj.SetOrientation(orientation);
        roll = obj.GetOrientation();
        roll[2] = roll[2] - 1;
        obj.SetOrientation(roll);
        roll[2] = roll[2] + 1;
        obj.SetOrientation(roll);
        obj.Update();
        vehicle.Synchronize();

        return vehicle;
    }

    protected void ApplyFluids(Transport vehicle, TXGVehicleData record, TXGSettings settings)
    {
        if (!vehicle || !record)
            return;

        bool saveFuel = (!settings || settings.saveVehicleFuel);
        float fuelFrac = 1.0;
        if (saveFuel)
            fuelFrac = record.fuelFraction;

        CarScript car = CarScript.Cast(vehicle);
        if (car)
        {
            float oilFrac = 1.0;
            float coolantFrac = 1.0;
            float brakeFrac = 1.0;
            if (saveFuel && record.fluidsSaved)
            {
                oilFrac = record.oilFraction;
                coolantFrac = record.coolantFraction;
                brakeFrac = record.brakeFraction;
            }
            RefillCarFluid(car, CarFluid.FUEL, fuelFrac);
            RefillCarFluid(car, CarFluid.OIL, oilFrac);
            RefillCarFluid(car, CarFluid.COOLANT, coolantFrac);
            RefillCarFluid(car, CarFluid.BRAKE, brakeFrac);
            return;
        }

        BoatScript boat = BoatScript.Cast(vehicle);
        if (boat)
        {
            boat.LeakAll(BoatFluid.FUEL);
            float boatCap = boat.GetFluidCapacity(BoatFluid.FUEL);
            if (boatCap > 0 && fuelFrac > 0)
                boat.Fill(BoatFluid.FUEL, boatCap * fuelFrac);
            return;
        }

        MotorbikeScript bike = MotorbikeScript.Cast(vehicle);
        if (bike)
        {
            bike.LeakAll(ETransportFluid.FUEL);
            float bikeCap = bike.GetFluidCapacity(ETransportFluid.FUEL);
            if (bikeCap > 0 && fuelFrac > 0)
                bike.FillScript(ETransportFluid.FUEL, bikeCap * fuelFrac);
            return;
        }
    }

    protected void RefillCarFluid(CarScript car, CarFluid fluid, float fraction)
    {
        car.LeakAll(fluid);
        if (fraction <= 0)
            return;
        float cap = car.GetFluidCapacity(fluid);
        if (cap > 0)
            car.Fill(fluid, cap * Math.Clamp(fraction, 0.0, 1.0));
    }

    // ============================================================
    //  SERVER: vehicle discovery / occupancy
    // ============================================================

    Transport FindNearbyTransport(vector pos, float radius)
    {
        array<Object> objects = new array<Object>();
        array<CargoBase> proxy = new array<CargoBase>();
        GetGame().GetObjectsAtPosition3D(pos, radius, objects, proxy);

        Transport best;
        float bestDist = radius + 1.0;
        foreach (Object o : objects)
        {
            Transport t = Transport.Cast(o);
            if (!t)
                continue;
            float d = vector.Distance(pos, t.GetPosition());
            if (d < bestDist)
            {
                bestDist = d;
                best = t;
            }
        }
        return best;
    }

    CarScript FindNearbyCar(vector pos, float radius)
    {
        return CarScript.Cast(FindNearbyTransport(pos, radius));
    }

    bool IsTransportOccupied(Transport vehicle)
    {
        if (!vehicle)
            return false;
        for (int i = 0; i < vehicle.CrewSize(); i++)
        {
            if (vehicle.CrewMember(i))
                return true;
        }
        return false;
    }

    bool IsCarOccupied(CarScript car)
    {
        return IsTransportOccupied(car);
    }

    // ============================================================
    //  SERVER: fees  — provider first, cash fallback
    // ============================================================

    // Bank first (if enabled and the balance covers it), then cash. With TraderX loaded the cash
    // side uses the core currency service (stacks + change); standalone uses the garage table.
    bool ChargeFee(PlayerBase player, int amount, string label)
    {
        if (amount <= 0)
            return true;

        TXGSettings settings = GetTXGConfigService().GetServerSettings();

#ifdef TraderX
        if (!settings || settings.payWithBank)
        {
            ITraderXPaymentProvider pay = TraderXProviderRegistry.GetInstance().GetBankingProvider();
            if (pay && pay.IsAvailable(player) && pay.DeductLabeled(player, amount, label, "Garage"))
                return true;
        }
        TraderXCurrencyService currencyService = GetTraderXCurrencyService();
        if (currencyService)
            return currencyService.RemoveMoneyAmountFromPlayer(player, amount);
#endif

        return RemoveCashFromPlayer(player, amount, settings);
    }

    // Standalone cash: counts stack quantities, takes smallest notes first, gives change back.
    protected bool RemoveCashFromPlayer(PlayerBase player, int amount, TXGSettings settings)
    {
        if (!settings || !player)
            return false;

        array<EntityAI> items = new array<EntityAI>();
        player.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, items);

        array<ItemBase> money = new array<ItemBase>();
        int total = 0;
        foreach (EntityAI entity : items)
        {
            ItemBase note = ItemBase.Cast(entity);
            if (!note || note == player)
                continue;
            int value = settings.GetCurrencyValue(note.GetType());
            if (value <= 0)
                continue;
            money.Insert(note);
            total = total + value * CashCount(note);
        }
        if (total < amount)
            return false;

        int remaining = amount;
        foreach (TXGCurrencyEntry denom : settings.GetCurrenciesSortedAsc())
        {
            foreach (ItemBase stack : money)
            {
                if (remaining <= 0)
                    break;
                if (!stack || stack.GetType() != denom.className)
                    continue;
                int count = CashCount(stack);
                // Integer ceil: remaining / denom.value would floor first (50 with a 100 note -> 0 taken).
                int need = (remaining + denom.value - 1) / denom.value;
                int take = Math.Min(need, count);
                remaining = remaining - take * denom.value;
                if (take >= count)
                    GetGame().ObjectDelete(stack);
                else
                    stack.SetQuantity(stack.GetQuantity() - take);
            }
        }

        if (remaining > 0)
        {
            GetTXGLog().LogError("[TXG] Cash payment incomplete, " + remaining.ToString() + " still due");
            return false;
        }
        if (remaining < 0)
            GiveCash(player, -remaining, settings);
        return true;
    }

    protected int CashCount(ItemBase note)
    {
        if (note.HasQuantity())
            return Math.Max(1, Math.Round(note.GetQuantity()));
        return 1;
    }

    protected void GiveCash(PlayerBase player, int amount, TXGSettings settings)
    {
        array<ref TXGCurrencyEntry> sorted = settings.GetCurrenciesSortedAsc();
        for (int i = sorted.Count() - 1; i >= 0 && amount > 0; i--)
        {
            TXGCurrencyEntry denom = sorted[i];
            int count = amount / denom.value;
            if (count <= 0)
                continue;
            amount = amount - count * denom.value;
            EntityAI created = player.GetInventory().CreateInInventory(denom.className);
            if (!created)
                created = EntityAI.Cast(GetGame().CreateObjectEx(denom.className, player.GetPosition(), ECE_PLACE_ON_SURFACE));
            ItemBase createdItem = ItemBase.Cast(created);
            if (createdItem && createdItem.HasQuantity())
                createdItem.SetQuantity(count);
            else
            {
                for (int extra = 1; extra < count; extra++)
                {
                    if (!player.GetInventory().CreateInInventory(denom.className))
                        GetGame().CreateObjectEx(denom.className, player.GetPosition(), ECE_PLACE_ON_SURFACE);
                }
            }
        }
    }

    // ============================================================
    //  SERVER: build the client list + health helpers
    // ============================================================

    array<ref TXGVehicleRowVM> BuildVehicleList(TXGGarageData data)
    {
        array<ref TXGVehicleRowVM> rows = new array<ref TXGVehicleRowVM>();
        if (!data)
            return rows;

        TXGSettings settings = GetTXGConfigService().GetServerSettings();
        TXGPlayerPolicies policies = TXGPlayerPolicies.Load(data.ownerUID);
        int nowUnix = TXGInsuranceService.GetNowUnix();
        policies.RefreshExpirations(nowUnix);

        foreach (TXGVehicleData v : data.vehicles)
        {
            if (!v)
                continue;
            rows.Insert(BuildRowVM(v, data.ownerUID, false, settings, policies, nowUnix));
        }

        // Vehicles other owners shared with this player (reverse index, no scan of every garage).
        array<string> owners = ShareIndexOwnersOf(data.ownerUID);
        foreach (string ownerUid : owners)
        {
            if (ownerUid == data.ownerUID || !FileExist(TXGGarageData.GetFilePath(ownerUid)))
                continue;
            TXGGarageData otherData = TXGGarageData.Load(ownerUid, "");
            TXGPlayerPolicies otherPols;
            foreach (TXGVehicleData sv : otherData.vehicles)
            {
                if (!sv || !sv.sharedWith || sv.sharedWith.Find(data.ownerUID) == -1)
                    continue;
                if (!otherPols)
                    otherPols = TXGPlayerPolicies.Load(ownerUid);
                rows.Insert(BuildRowVM(sv, ownerUid, true, settings, otherPols, nowUnix));
            }
        }

        return rows;
    }

    protected TXGVehicleRowVM BuildRowVM(TXGVehicleData v, string ownerUid, bool isSharedRow, TXGSettings settings, TXGPlayerPolicies policies, int nowUnix)
    {
        TXGVehicleRowVM row = new TXGVehicleRowVM();
        row.vehicleId = v.vehicleId;
        row.className = v.className;
        row.displayName = GetDisplayName(v.className);
        row.healthLevel = ConvertHealthToHealthLevel(v.className, v.health);
        row.healthText = GetHealthLevelText(row.healthLevel);
        row.fuelPercent = Math.Round(Math.Clamp(v.fuelFraction, 0.0, 1.0) * 100.0);
        row.oilPercent = Math.Round(Math.Clamp(v.oilFraction, 0.0, 1.0) * 100.0);
        row.coolantPercent = Math.Round(Math.Clamp(v.coolantFraction, 0.0, 1.0) * 100.0);
        row.brakePercent = Math.Round(Math.Clamp(v.brakeFraction, 0.0, 1.0) * 100.0);
        row.engineHealthLevel = GetZoneHealthLevel(v, "Engine", row.healthLevel);
        row.chassisHealthLevel = GetZoneHealthLevel(v, "Chassis", row.healthLevel);
        row.hasCarLock = v.hasCarLock;
        row.holdReason = v.holdReason;
        row.releaseFee = v.releaseFee;
        row.isShared = isSharedRow;
        if (v.sharedWith)
            row.sharedCount = v.sharedWith.Count();

        row.repairQuote = ComputeRepairQuote(v, settings);
        if (!isSharedRow && v.sharedWith)
            row.sharedWith.Copy(v.sharedWith);

        TXGInsuranceService insSvc = GetTXGInsuranceService();
        if (insSvc && settings && settings.insurance && settings.insurance.enabled)
        {
            int catVal = insSvc.GetVehicleCatalogValue(v.className);
            if (catVal > 0)
            {
                row.insuranceCoveredValue = catVal;
                row.insurancePerDay = insSvc.ComputePremium(catVal, 1, settings.insurance);
                row.insurancePremiumQuote = insSvc.ComputePremium(catVal, settings.insurance.minDays, settings.insurance);
                row.insuranceDeductibleQuote = insSvc.ComputeDeductibleFor(v.className, catVal, settings.insurance);
            }
            if (policies && v.policyId != "")
            {
                TXGInsurancePolicy pol = policies.FindPolicy(v.policyId);
                if (pol && pol.status == TXGConstants.POLICY_ACTIVE && pol.expiresAt > nowUnix)
                {
                    row.isInsured = true;
                    row.insuranceExpiresAt = pol.expiresAt;
                    row.insuranceDaysLeft = Math.Ceil((pol.expiresAt - nowUnix) / 86400.0);
                }
            }
        }

        if (v.snapshot && v.snapshot.childrens)
        {
            int cargoCnt = 0;
            foreach (TXGItemSnapshot child : v.snapshot.childrens)
            {
                if (!child || child.item == "")
                    continue;
                if (child.isAttachment)
                    row.attachmentClasses.Insert(child.item);
                else
                    cargoCnt = cargoCnt + 1;
            }
            row.cargoCount = cargoCnt;
        }
        return row;
    }

    // ============================================================
    //  SERVER: REPAIR, SHARING, ADMIN IMPOUND & ZONES
    // ============================================================

    int ComputeRepairQuote(TXGVehicleData v, TXGSettings settings)
    {
        if (!v || !settings || !settings.enableRepairs)
            return 0;

        int total = 0;
        if (v.fuelFraction < 0.99 || v.oilFraction < 0.99 || v.coolantFraction < 0.99 || v.brakeFraction < 0.99)
            total = total + settings.repairFluidCost;

        float maxHp = MiscGameplayFunctions.GetTypeMaxGlobalHealth(v.className);
        bool bodyDamaged = (maxHp > 0 && v.health < maxHp * 0.99);
        if (!bodyDamaged && v.zones)
        {
            foreach (TXGZoneHealth zh : v.zones)
            {
                if (zh && zh.health01 < 0.99)
                {
                    bodyDamaged = true;
                    break;
                }
            }
        }
        if (bodyDamaged)
        {
            int bodyFee = settings.repairBodyCost;
            TXGInsuranceService insSvc = GetTXGInsuranceService();
            if (insSvc && settings.repairBodyPercent > 0)
            {
                int val = insSvc.GetVehicleCatalogValue(v.className);
                if (val > 0)
                    bodyFee = bodyFee + Math.Ceil(val * (settings.repairBodyPercent * 0.01));
            }
            total = total + bodyFee;
        }

        if (v.snapshot && v.snapshot.childrens)
        {
            foreach (TXGItemSnapshot att : v.snapshot.childrens)
            {
                if (!att || !att.isAttachment || att.item == "")
                    continue;
                float attMax = MiscGameplayFunctions.GetTypeMaxGlobalHealth(att.item);
                if (attMax <= 0)
                    continue;
                if (att.health <= 0.01)
                    total = total + settings.repairRuinedPartCost;
                else if (att.health < attMax * 0.99)
                    total = total + settings.repairWornPartCost;
            }
        }
        return total;
    }

    string RepairStoredVehicle(PlayerBase player, string vehicleId, out bool success)
    {
        success = false;
        if (!GetGame().IsServer() || !player || !player.GetIdentity())
            return "#txg_err_server";

        TXGSettings settings = GetTXGConfigService().GetServerSettings();
        if (!settings || !settings.enableRepairs)
            return "#txg_err_service_disabled";

        string uid = player.GetIdentity().GetPlainId();
        string name = player.GetIdentity().GetName();

        if (!IsAtGarage(player))
            return "#txg_err_not_at_garage";

        TXGGarageData data = TXGGarageData.Load(uid, name);
        TXGVehicleData v = data.FindVehicle(vehicleId);
        if (!v)
            return "#txg_err_vehicle_not_found";

        int quote = ComputeRepairQuote(v, settings);
        if (quote <= 0)
            return "#txg_err_already_pristine";

        if (!ChargeFee(player, quote, TXGConstants.FEE_LABEL_REPAIR))
            return TXGText.Msg("#txg_err_no_money_service", quote.ToString());

        float maxHp = MiscGameplayFunctions.GetTypeMaxGlobalHealth(v.className);
        if (maxHp > 0)
            v.health = maxHp;

        if (v.zones)
        {
            foreach (TXGZoneHealth zh : v.zones)
            {
                if (zh)
                    zh.health01 = 1.0;
            }
        }

        v.fuelFraction = 1.0;
        v.oilFraction = 1.0;
        v.coolantFraction = 1.0;
        v.brakeFraction = 1.0;
        v.fluidsSaved = true;

        if (v.snapshot && v.snapshot.childrens)
        {
            foreach (TXGItemSnapshot att : v.snapshot.childrens)
            {
                if (!att || !att.isAttachment || att.item == "")
                    continue;
                float attMax = MiscGameplayFunctions.GetTypeMaxGlobalHealth(att.item);
                if (attMax > 0)
                    att.health = attMax;
            }
        }

        data.Save();
        GetTXGLog().LogInfo("[TXG] Repair: " + name + " serviced " + v.className + " (" + vehicleId + ") for " + quote.ToString());
        success = true;
        return TXGText.Msg("#txg_msg_serviced", quote.ToString());
    }

    // mode: 1 = add, 2 = remove, 0 = toggle.
    string ShareStoredVehicle(PlayerBase player, string vehicleId, string targetUid, int mode, out bool success)
    {
        success = false;
        if (!GetGame().IsServer() || !player || !player.GetIdentity())
            return "#txg_err_server";

        targetUid.TrimInPlace();
        if (!IsSteam64(targetUid))
            return "#txg_err_steam_id";

        if (!IsAtGarage(player))
            return "#txg_err_not_at_garage";

        string uid = player.GetIdentity().GetPlainId();
        if (targetUid == uid)
            return "#txg_err_own_vehicle";

        TXGGarageData data = TXGGarageData.Load(uid, player.GetIdentity().GetName());
        TXGVehicleData v = data.FindVehicle(vehicleId);
        if (!v)
            return "#txg_err_only_owner_share";

        if (!v.sharedWith)
            v.sharedWith = new array<string>();

        int existingIdx = v.sharedWith.Find(targetUid);
        if (mode == 1 && existingIdx != -1)
        {
            success = true;
            return "#txg_err_already_shared";
        }
        if (mode == 2 && existingIdx == -1)
        {
            success = true;
            return "#txg_err_access_already_removed";
        }
        if (existingIdx != -1)
        {
            v.sharedWith.RemoveOrdered(existingIdx);
            data.Save();
            success = true;
            return TXGText.Msg("#txg_msg_access_removed", PlayerNameOf(targetUid));
        }

        v.sharedWith.Insert(targetUid);
        data.Save();
        ShareIndexAdd(uid, v.sharedWith);
        success = true;
        return TXGText.Msg("#txg_msg_access_given", PlayerNameOf(targetUid));
    }

    protected bool FindSharedVehicle(string requesterUid, string vehicleId, out TXGGarageData outOwnerData, out TXGVehicleData outRecord)
    {
        outOwnerData = null;
        outRecord = null;
        foreach (string ownerUid : ShareIndexOwnersOf(requesterUid))
        {
            if (ownerUid == requesterUid || !FileExist(TXGGarageData.GetFilePath(ownerUid)))
                continue;
            TXGGarageData otherData = TXGGarageData.Load(ownerUid, "");
            foreach (TXGVehicleData v : otherData.vehicles)
            {
                if (v && v.vehicleId == vehicleId && v.sharedWith && v.sharedWith.Find(requesterUid) != -1)
                {
                    outOwnerData = otherData;
                    outRecord = v;
                    return true;
                }
            }
        }
        return false;
    }

    // guest uid -> owners who shared at least one stored vehicle with them. Built by the boot scan,
    // kept up to date on share/return; entries are only hints, the owner's file is re-checked.
    protected ref map<string, ref array<string>> m_ShareIndex = new map<string, ref array<string>>();

    void ShareIndexAdd(string ownerUid, array<string> guests)
    {
        if (!guests)
            return;
        foreach (string guest : guests)
        {
            array<string> owners = m_ShareIndex.Get(guest);
            if (!owners)
            {
                owners = new array<string>();
                m_ShareIndex.Set(guest, owners);
            }
            if (owners.Find(ownerUid) == -1)
                owners.Insert(ownerUid);
        }
    }

    array<string> ShareIndexOwnersOf(string guestUid)
    {
        array<string> owners = m_ShareIndex.Get(guestUid);
        if (!owners)
            owners = new array<string>();
        return owners;
    }

    static const float SHARE_NEARBY_RADIUS = 50.0;

    // Share panel source: online players within 50 m + everyone the owner shared a vehicle with.
    void BuildPeople(PlayerBase player, TXGGarageData data, array<ref TXGSharePlayerVM> outPeople)
    {
        if (!player || !player.GetIdentity() || !outPeople)
            return;
        // Nearby names/uids/distances only at a garage: RequestGarage can be sent from anywhere.
        if (!IsAtGarage(player))
            return;
        string selfUid = player.GetIdentity().GetPlainId();

        array<Man> players = new array<Man>();
        GetGame().GetPlayers(players);
        foreach (Man m : players)
        {
            if (!m || !m.GetIdentity() || m == player)
                continue;
            float d = vector.Distance(m.GetPosition(), player.GetPosition());
            if (d > SHARE_NEARBY_RADIUS)
                continue;
            TXGSharePlayerVM near = new TXGSharePlayerVM();
            near.uid = m.GetIdentity().GetPlainId();
            near.name = m.GetIdentity().GetName();
            near.distance = Math.Round(d);
            outPeople.Insert(near);
        }

        if (!data)
            return;
        foreach (TXGVehicleData v : data.vehicles)
        {
            if (!v || !v.sharedWith)
                continue;
            foreach (string uid : v.sharedWith)
            {
                if (uid == "" || uid == selfUid || FindPerson(outPeople, uid))
                    continue;
                TXGSharePlayerVM known = new TXGSharePlayerVM();
                known.uid = uid;
                known.name = PlayerNameOf(uid);
                outPeople.Insert(known);
            }
        }
    }

    protected static TXGSharePlayerVM FindPerson(array<ref TXGSharePlayerVM> people, string uid)
    {
        foreach (TXGSharePlayerVM p : people)
        {
            if (p && p.uid == uid)
                return p;
        }
        return null;
    }

    // Last known name (garage index), else a shortened Steam64.
    static string PlayerNameOf(string uid)
    {
        TXGPlayerIndex idx = TXGPlayerIndex.Get();
        if (idx && idx.entries)
        {
            foreach (TXGPlayerIndexEntry e : idx.entries)
            {
                if (e && e.uid == uid && e.name != "")
                    return e.name;
            }
        }
        array<Man> players = new array<Man>();
        GetGame().GetPlayers(players);
        foreach (Man m : players)
        {
            if (m && m.GetIdentity() && m.GetIdentity().GetPlainId() == uid)
                return m.GetIdentity().GetName();
        }
        return "..." + uid.Substring(uid.Length() - 4, 4);
    }

    void GetBalances(PlayerBase player, out int bank, out int cash)
    {
        bank = -1;
        cash = 0;
        TXGSettings settings = GetTXGConfigService().GetServerSettings();
#ifdef TraderX
        if (!settings || settings.payWithBank)
        {
            ITraderXPaymentProvider pay = TraderXProviderRegistry.GetInstance().GetBankingProvider();
            if (pay && pay.IsAvailable(player))
                bank = pay.GetBalance(player);
        }
        TraderXCurrencyService currencyService = GetTraderXCurrencyService();
        if (currencyService)
        {
            cash = currencyService.GetPlayerMoneyFromAllCurrency(player);
            return;
        }
#endif
        cash = CountStandaloneCash(player, settings);
    }

    protected int CountStandaloneCash(PlayerBase player, TXGSettings settings)
    {
        if (!settings || !player)
            return 0;
        array<EntityAI> items = new array<EntityAI>();
        player.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, items);
        int total = 0;
        foreach (EntityAI entity : items)
        {
            ItemBase note = ItemBase.Cast(entity);
            if (note && note != player)
                total = total + settings.GetCurrencyValue(note.GetType()) * CashCount(note);
        }
        return total;
    }

    static bool IsSteam64(string uid)
    {
        if (uid.Length() != 17 || uid.IndexOf("7656") != 0)
            return false;
        for (int i = 0; i < uid.Length(); i++)
        {
            string c = uid.Get(i);
            if ("0123456789".IndexOf(c) == -1)
                return false;
        }
        return true;
    }

    bool IsPlayerAdmin(string uid)
    {
        if (uid == "")
            return false;
        TXGSettings settings = GetTXGConfigService().GetServerSettings();
        if (!settings || !settings.adminSteamIds)
            return false;
        return settings.adminSteamIds.Find(uid) != -1;
    }

    string AdminImpoundNearby(PlayerBase admin, out bool success)
    {
        success = false;
        if (!GetGame().IsServer() || !admin || !admin.GetIdentity())
            return "#txg_err_server";

        string adminUid = admin.GetIdentity().GetPlainId();
        if (!IsPlayerAdmin(adminUid))
            return "#txg_err_admin_required";

        TXGSettings settings = GetTXGConfigService().GetServerSettings();
        if (!settings)
            return "#txg_err_not_configured";

        Transport vehicle = FindNearbyTransport(admin.GetPosition(), settings.parkRadius);
        if (!vehicle)
            return "#txg_err_no_nearby_impound";

        string targetOwner = "";
        string stampPol;
        int stampState;
        int stampResale;
        TXGVehicleStampHelper.ReadStamp(vehicle, stampPol, targetOwner, stampState, stampResale);
#ifdef TraderX
        ITraderXCarLockProvider lockProv = TraderXProviderRegistry.GetInstance().GetCarLockProvider();
        if (targetOwner == "" && lockProv)
            targetOwner = lockProv.GetOwner(vehicle);
#endif
        // An ownerless vehicle would land in the admin's own garage: refuse instead.
        if (targetOwner == "")
            return "#txg_err_no_owner";

        TXGGarageData data = TXGGarageData.Load(targetOwner, "");
        TXGVehicleData record = CaptureVehicleRecord(data, vehicle, targetOwner, settings);
        record.holdReason = TXGConstants.HOLD_IMPOUND;
        record.releaseFee = settings.impoundFee;

        data.vehicles.Insert(record);
        data.Save();

        TXGVehicleStampHelper.SetGarageRemoval(vehicle, true);
        GetGame().ObjectDelete(vehicle);

        GetTXGLog().LogInfo("[TXG ADMIN] IMPOUND admin=" + adminUid + " owner=" + targetOwner + " class=" + record.className + " fee=" + record.releaseFee.ToString());
        success = true;
        return TXGText.Msg("#txg_msg_impounded", record.className, targetOwner);
    }

    bool IsInNoRetrieveZone(vector pos, TXGSettings settings, out string zoneName)
    {
        zoneName = "";
        if (!settings || !settings.noRetrieveZones)
            return false;
        foreach (TXGZoneEntry z : settings.noRetrieveZones)
        {
            if (!z || z.radius <= 0)
                continue;
            float dx = pos[0] - z.position[0];
            float dz = pos[2] - z.position[2];
            if ((dx * dx + dz * dz) <= (z.radius * z.radius))
            {
                zoneName = z.name;
                if (zoneName == "")
                    zoneName = "#txg_zone_restricted";
                return true;
            }
        }
        return false;
    }

    static int GetZoneHealthLevel(TXGVehicleData v, string zoneName, int fallbackLevel)
    {
        if (!v || !v.zones)
            return fallbackLevel;
        foreach (TXGZoneHealth zh : v.zones)
        {
            if (zh && zh.zone == zoneName)
            {
                if (zh.health01 > 0.7) return TXGConstants.HEALTH_PRISTINE;
                if (zh.health01 > 0.5) return TXGConstants.HEALTH_WORN;
                if (zh.health01 > 0.3) return TXGConstants.HEALTH_DAMAGED;
                if (zh.health01 > 0.0) return TXGConstants.HEALTH_BADLY_DAMAGED;
                return TXGConstants.HEALTH_RUINED;
            }
        }
        return fallbackLevel;
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
        if (level == TXGConstants.HEALTH_PRISTINE)      return "#txg_health_pristine";
        if (level == TXGConstants.HEALTH_WORN)          return "#txg_health_worn";
        if (level == TXGConstants.HEALTH_DAMAGED)       return "#txg_health_damaged";
        if (level == TXGConstants.HEALTH_BADLY_DAMAGED) return "#txg_health_badly_damaged";
        return "#txg_health_ruined";
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
        TXGVehicleData record = CaptureVehicleRecord(data, vehicle, uid, settings);

        data.vehicles.Insert(record);
        data.Save();

        TXGVehicleStampHelper.SetGarageRemoval(vehicle, true);
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

        int skippedItems = 0;
        EntityAI vehicle = CreateVehicle(record, pos, orientation, uid, skippedItems);
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
