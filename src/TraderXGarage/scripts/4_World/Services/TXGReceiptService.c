// TraderXGarage - Vehicle receipts (server-authoritative)
// Pack: a vehicle becomes a receipt item. Its full state (attachments, cargo, fluids, damage zones,
// CarLock) is captured like a garage park-in and kept in TraderXGarage/Receipts/<id>.json.
// Deploy: the receipt becomes the vehicle again in front of the player, who becomes its owner.
// Receipts packed by TraderPlus V1 (NewReceipt with inline data, older Receipt) deploy too.
// The actions only call in: every rule is checked here.
class TXGReceiptService extends DayZularService
{
    void TXGReceiptService()
    {
        if (GetGame().IsClient() || !GetGame().IsMultiplayer())
        {
            GetRPCManager().AddRPC(TXGConstants.RPC_NAMESPACE, "OnReceiptClass", this, SingleplayerExecutionType.Client);
            GetRPCManager().AddRPC(TXGConstants.RPC_NAMESPACE, "OnReceiptStatus", this, SingleplayerExecutionType.Client);
        }
    }

    // ============================================================
    //  CLIENT
    // ============================================================

    void OnReceiptClass(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type != CallType.Client)
            return;

        Param1<string> data = new Param1<string>("");
        if (!ctx.Read(data))
            return;

#ifndef TraderPlus
        NewReceiptBase receipt = NewReceiptBase.Cast(target);
        if (receipt)
            receipt.CarClassName = data.param1;
#endif
    }

    void OnReceiptStatus(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type != CallType.Client)
            return;

        Param1<string> data = new Param1<string>("");
        if (!ctx.Read(data))
            return;

        NotificationSystem.AddNotificationExtended(6, TXGText.Get("#txg_receipt"), TXGText.Localize(data.param1), "set:dayz_gui image:icon_info");
    }

    // ============================================================
    //  SERVER
    // ============================================================

    void TellPlayer(PlayerBase player, string msg)
    {
        if (player && player.GetIdentity())
            GetRPCManager().SendRPC(TXGConstants.RPC_NAMESPACE, "OnReceiptStatus", new Param1<string>(msg), true, player.GetIdentity());
    }

    protected TXGReceiptSettings GetReceiptSettings()
    {
        TXGSettings settings = GetTXGConfigService().GetServerSettings();
        if (!settings)
            return null;
        if (!settings.receipts)
            settings.receipts = new TXGReceiptSettings();
        return settings.receipts;
    }

    // TraderPlus allowed receipts within 50 m of a trader; garages count as traders here
    bool IsNearTrader(vector pos, float distance)
    {
        TXGSettings settings = GetTXGConfigService().GetServerSettings();
        if (settings)
        {
            foreach (TXGNpcEntry npc : settings.npcs)
            {
                if (npc && vector.Distance(npc.position, pos) <= distance)
                    return true;
            }
        }
#ifdef TraderX
        if (GetTraderXModule() && GetTraderXModule().GetSettings() && GetTraderXModule().GetSettings().traders)
        {
            foreach (TraderXNpc trader : GetTraderXModule().GetSettings().traders)
            {
                if (trader && vector.Distance(trader.position, pos) <= distance)
                    return true;
            }
        }
#endif
        return false;
    }

    protected bool HasCargo(EntityAI vehicle)
    {
        CargoBase cargo = vehicle.GetInventory().GetCargo();
        return cargo && cargo.GetItemCount() > 0;
    }

    protected string NewReceiptId(string uid)
    {
        string id = uid + "-" + TXGGarageService.GetUtcStamp().ToString() + "-" + Math.RandomIntInclusive(100000, 999999).ToString();
        while (FileExist(TXGReceiptData.GetFilePath(id)))
            id = id + Math.RandomIntInclusive(0, 9).ToString();
        return id;
    }

#ifndef TraderPlus
    // --- Pack ------------------------------------------------------------------------------------

    string Pack(PlayerBase player, CarScript car, ItemBase tool, out bool success)
    {
        success = false;
        if (!GetGame().IsServer() || !player || !player.GetIdentity() || !player.IsAlive())
            return "#txg_err_server";

        TXGReceiptSettings receipts = GetReceiptSettings();
        if (!receipts || !receipts.enabled)
            return "#txg_receipt_err_disabled";

        if (!tool || tool != player.GetItemInHands() || !tool.IsKindOf("Wrench") || tool.IsRuined())
            return "#txg_receipt_err_wrench";

        if (!car || vector.Distance(car.GetPosition(), player.GetPosition()) > 8.0)
            return TXGText.Msg("#txg_err_no_vehicle_within", "8");

        if (receipts.traderOnly && !IsNearTrader(player.GetPosition(), receipts.traderDistance))
            return TXGText.Msg("#txg_receipt_err_trader", Math.Round(receipts.traderDistance).ToString());

        PackVehicle(car, player.GetIdentity().GetPlainId(), player.GetIdentity().GetName(), player, success);
        return m_LastStatus;
    }

    protected string m_LastStatus;

    // Packs the vehicle for the player uid: vehicle rules only (Pack checks the player side).
    // The receipt goes to the player's inventory, or to the ground under the player when full.
    NewReceiptBase PackVehicle(CarScript car, string uid, string name, PlayerBase player, out bool success)
    {
        success = false;
        m_LastStatus = "#txg_err_server";
        TXGReceiptSettings receipts = GetReceiptSettings();
        TXGGarageService garage = GetTXGGarageService();
        if (!receipts || !garage || !car)
            return null;

        if (car.IsRuined())
        {
            m_LastStatus = "#txg_err_destroyed";
            return null;
        }
        if (garage.IsTransportOccupied(car))
        {
            m_LastStatus = "#txg_err_occupied";
            return null;
        }

        string stampPolicy;
        string stampOwner;
        int stampState;
        int stampNoResale;
        TXGVehicleStampHelper.ReadStamp(car, stampPolicy, stampOwner, stampState, stampNoResale);
        if (stampState == TXGConstants.STAMP_CLAIMED)
        {
            m_LastStatus = "#txg_err_already_replaced";
            return null;
        }

#ifdef TraderX
        // As in TraderPlus: only the owner of the vehicle's lock packs it (an unlocked stranger's car cannot be taken)
        ITraderXCarLockProvider lockProv = TraderXProviderRegistry.GetInstance().GetCarLockProvider();
        if (lockProv)
        {
            if (!lockProv.HasCarLock(car))
            {
                m_LastStatus = "#txg_err_need_carlock";
                return null;
            }
            if (!lockProv.IsOwner(car, uid))
            {
                m_LastStatus = "#txg_err_not_owner";
                return null;
            }
        }
#endif
        string lockError;
        if (!garage.CanPackReceipt(car, uid, lockError))
        {
            m_LastStatus = lockError;
            return null;
        }

        if (!receipts.saveCargo && HasCargo(car))
        {
            m_LastStatus = "#txg_receipt_err_cargo";
            return null;
        }

        string receiptId = NewReceiptId(uid);
        TXGReceiptData data = new TXGReceiptData();
        data.receiptId = receiptId;
        data.packerUid = uid;
        data.packerName = name;
        data.packedAt = TXGGarageService.GetUtcStamp();
        data.vehicle = garage.CaptureReceiptRecord(car, uid, receiptId);
        // Without saveLock the code is dropped; the lock itself goes to whoever takes the vehicle out (as TraderPlus)
        if (!receipts.saveLock)
            data.vehicle.lockCode = -1;
        if (!data.Save())
        {
            m_LastStatus = "#txg_err_server";
            return null;
        }

        EntityAI item;
        if (player)
            item = player.GetHumanInventory().CreateInInventory("NewReceipt");
        if (!item)
        {
            vector dropPos = car.GetPosition();
            if (player)
                dropPos = player.GetPosition();
            item = EntityAI.Cast(GetGame().CreateObjectEx("NewReceipt", dropPos, ECE_PLACE_ON_SURFACE));
        }

        NewReceiptBase receipt = NewReceiptBase.Cast(item);
        if (!receipt)
        {
            if (item)
                GetGame().ObjectDelete(item);
            TXGReceiptData.Delete(receiptId);
            return null;
        }

        receipt.m_TXG_ReceiptId = receiptId;
        receipt.CarClassName = car.GetType();
        receipt.LockCargo();
        if (player && player.GetIdentity())
            GetRPCManager().SendRPC(TXGConstants.RPC_NAMESPACE, "OnReceiptClass", new Param1<string>(receipt.CarClassName), true, player.GetIdentity(), receipt);

        GetTXGLog().LogInfo("[TXG] Receipt: " + name + " (" + uid + ") packed " + car.GetType() + " at " + car.GetPosition().ToString() + " into " + receiptId);
        TXGVehicleStampHelper.SetGarageRemoval(car, true);
        GetGame().ObjectDelete(car);

        success = true;
        m_LastStatus = "#txg_receipt_packed";
        return receipt;
    }

    // --- Deploy ----------------------------------------------------------------------------------

    string Deploy(PlayerBase player, ItemBase item, out bool success)
    {
        success = false;
        if (!GetGame().IsServer() || !player || !player.GetIdentity() || !player.IsAlive())
            return "#txg_err_server";

        if (!item || item != player.GetItemInHands())
            return "#txg_receipt_err_hands";

        TXGReceiptSettings receipts = GetReceiptSettings();
        if (!receipts)
            return "#txg_err_not_configured";

        if (receipts.traderOnly && !IsNearTrader(player.GetPosition(), receipts.traderDistance))
            return TXGText.Msg("#txg_receipt_err_trader", Math.Round(receipts.traderDistance).ToString());

        vector dir = player.GetDirection();
        dir[1] = 0;
        dir.Normalize();
        vector pos = player.GetPosition() + dir * receipts.deployDistance;
        vector playerOri = player.GetOrientation();
        vector ori = Vector(playerOri[0], 0, 0);

        DeployAt(item, pos, ori, player.GetIdentity().GetPlainId(), player.GetIdentity().GetName(), success);
        return m_LastStatus;
    }

    // Builds the vehicle of the receipt at pos (ground height is resolved here) for uid, then deletes
    // the receipt. The spot must be clear of objects and players.
    EntityAI DeployAt(ItemBase item, vector pos, vector ori, string uid, string name, out bool success)
    {
        success = false;
        m_LastStatus = "#txg_err_server";
        TXGReceiptSettings receipts = GetReceiptSettings();
        TXGGarageService garage = GetTXGGarageService();
        if (!receipts || !garage || !item)
            return null;

        pos[1] = GetGame().SurfaceY(pos[0], pos[2]);
        if (GetGame().SurfaceIsSea(pos[0], pos[2]) || GetGame().SurfaceIsPond(pos[0], pos[2]))
        {
            m_LastStatus = "#txg_receipt_err_spot";
            return null;
        }

        vector box = receipts.deployBoxSize;
        if (!garage.IsParkingAvailable(pos + Vector(0, box[1] * 0.5 + 0.2, 0), ori, box) || IsPlayerNear(pos, 3.0))
        {
            m_LastStatus = "#txg_receipt_err_spot";
            return null;
        }

        string receiptId = "";
        TXGVehicleData record = BuildRecord(item, uid, receiptId);
        if (!record)
            return null;

        if (!TXGGarageService.IsEntityClassDefined(record.className))
        {
            m_LastStatus = TXGText.Msg("#txg_err_mod_missing", record.className);
            return null;
        }

        TXGTraderPlusVehicleParts tpParts = TXGTraderPlusVehiclePartsConfig.Load().FindByClass(record.className);
        if (tpParts && tpParts.Height > 0)
            pos[1] = pos[1] + tpParts.Height;

        int skipped = 0;
        EntityAI vehicle = garage.CreateVehicle(record, pos, ori, uid, skipped);
        if (!vehicle)
        {
            m_LastStatus = "#txg_receipt_err_spot";
            return null;
        }

        GetTXGLog().LogInfo("[TXG] Receipt: " + name + " (" + uid + ") took out " + record.className + " at " + pos.ToString() + " (" + item.GetType() + " " + receiptId + ", skippedItems=" + skipped.ToString() + ")");

        GetGame().ObjectDelete(item);
        TXGReceiptData.Delete(receiptId);

        success = true;
        m_LastStatus = "#txg_receipt_deployed";
        if (skipped > 0)
            m_LastStatus = TXGText.Msg("#txg_msg_retrieved_skipped", skipped.ToString());
        return vehicle;
    }

    // TraderPlus parts are "class" or "class,health": fitted fresh (spawn quantity and energy), at that health
    protected void AddTraderPlusParts(TXGVehicleData record, array<string> parts)
    {
        if (!parts)
            return;

        foreach (string line : parts)
        {
            TStringArray fields = new TStringArray();
            line.Split(",", fields);
            if (fields.Count() < 1 || fields[0] == "")
                continue;

            TXGItemSnapshot part = new TXGItemSnapshot();
            part.item = fields[0];
            part.isAttachment = true;
            part.slot = -1;
            part.quantity = -1;
            part.energy = -1;
            part.health = -1;
            if (fields.Count() > 1 && fields[1].ToInt() != -1)
                part.health = fields[1].ToFloat();
            record.snapshot.childrens.Insert(part);
        }
    }

    string GetLastStatus()
    {
        return m_LastStatus;
    }

    protected bool IsPlayerNear(vector pos, float radius)
    {
        array<Object> objects = new array<Object>();
        GetGame().GetObjectsAtPosition3D(pos, radius, objects, null);
        foreach (Object o : objects)
        {
            if (o && o.IsInherited(SurvivorBase))
                return true;
        }
        return false;
    }

    // The vehicle record of any kind of receipt, owned by uid (the receipt is the vehicle's title).
    protected TXGVehicleData BuildRecord(ItemBase item, string uid, out string receiptId)
    {
        receiptId = "";
        TXGVehicleData record;

        NewReceiptBase receipt = NewReceiptBase.Cast(item);
        if (receipt && receipt.m_TXG_ReceiptId != "")
        {
            receiptId = receipt.m_TXG_ReceiptId;
            TXGReceiptData data = TXGReceiptData.Load(receiptId);
            if (!data)
            {
                // Already taken out (a duplicated item) or lost: the receipt is worthless
                m_LastStatus = "#txg_receipt_err_invalid";
                GetTXGLog().LogWarning("[TXG] Receipt " + receiptId + " has no vehicle record (duplicated or already used) - receipt deleted");
                GetGame().ObjectDelete(item);
                return null;
            }

            record = data.vehicle;
            if (record.policyId != "" && data.packerUid != uid)
            {
                TXGInsuranceService insSvc = GetTXGInsuranceService();
                if (insSvc)
                    insSvc.VoidPolicy(data.packerUid, record.policyId, "OWNER_CHANGED");
                record.policyId = "";
            }
            record.sharedWith.Clear();
            if (record.hasCarLock)
                record.lockOwnerUid = uid;
            return record;
        }

        // Packed by TraderPlus: a fresh vehicle with full fluids (as TraderPlus deployed it)
        record = new TXGVehicleData();
        record.vehicleId = "TP";
        record.health = -1;
        record.fuelFraction = 1.0;
        record.fluidsSaved = true;
        record.snapshot = new TXGItemSnapshot();

        if (receipt && receipt.CarClassName != "")
        {
            record.className = receipt.CarClassName;
            AddTraderPlusParts(record, receipt.Attachments);

            // Its cargo is the vehicle's cargo
            CargoBase cargo = receipt.GetInventory().GetCargo();
            if (cargo)
            {
                for (int i = 0; i < cargo.GetItemCount(); i++)
                {
                    EntityAI cargoItem = cargo.GetItem(i);
                    if (cargoItem)
                    {
                        TXGItemSnapshot snap = GetTXGGarageService().SnapshotItem(cargoItem);
                        snap.isAttachment = false;
                        record.snapshot.childrens.Insert(snap);
                    }
                }
            }

            // TraderPlus gave the lock to whoever deployed the receipt, with the saved code
            record.hasCarLock = true;
            record.lockOwnerUid = uid;
            record.lockCode = -1;
            if (receipt.CarPassword != 0)
                record.lockCode = receipt.CarPassword;
            return record;
        }

        ReceiptBase oldReceipt = ReceiptBase.Cast(item);
        if (oldReceipt)
        {
            TXGTraderPlusVehiclePartsConfig partsConfig = TXGTraderPlusVehiclePartsConfig.Load();
            if (oldReceipt.CarID >= 0 && oldReceipt.CarID < partsConfig.VehiclesParts.Count() && partsConfig.VehiclesParts[oldReceipt.CarID])
            {
                TXGTraderPlusVehicleParts entry = partsConfig.VehiclesParts[oldReceipt.CarID];
                record.className = entry.VehicleName;
                AddTraderPlusParts(record, entry.VehicleParts);
                return record;
            }
        }

        m_LastStatus = "#txg_receipt_err_invalid";
        return null;
    }
#endif
}

// Global accessor
static ref TXGReceiptService g_TXGReceiptService;

TXGReceiptService GetTXGReceiptService()
{
    if (!g_TXGReceiptService)
        g_TXGReceiptService = TXGReceiptService.Cast(DZlrServiceRegistry.GetInstance().GetService(TXGReceiptService));
    return g_TXGReceiptService;
}
