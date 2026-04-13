class TraderXReceiptService : ITraderXReceiptProvider
{
    private static ref TraderXReceiptService m_Instance;
    private ref TraderXReceiptSettings m_Settings;

    static ref ScriptInvoker Event_OnReceiptsReceived = new ScriptInvoker();
    static ref ScriptInvoker Event_OnRedeemResult = new ScriptInvoker();

    static TraderXReceiptService GetInstance()
    {
        if (!m_Instance)
            m_Instance = new TraderXReceiptService();
        return m_Instance;
    }

    void TraderXReceiptService()
    {
        m_Instance = this;
    }

    void Initialize()
    {
        m_Settings = TraderXReceiptSettings.Load();

        if (m_Settings.enabled)
        {
            TraderXProviderRegistry.GetInstance().RegisterReceiptProvider(this);
            GetTraderXLogger().LogInfo("[Receipt] Service initialized (expiry: " + m_Settings.receiptExpiryDays + " days, max: " + m_Settings.maxReceiptsPerPlayer + ")");
        }
    }

    void RegisterRPCs()
    {
        if (GetGame().IsServer())
        {
            GetDayZularRPCManager().AddRPC("TraderX", "GetPlayerReceiptsRequest", this, DayZularRPCType.Server);
            GetDayZularRPCManager().AddRPC("TraderX", "RedeemReceiptRequest", this, DayZularRPCType.Server);
        }
        else
        {
            GetDayZularRPCManager().AddRPC("TraderX", "GetPlayerReceiptsResponse", this, DayZularRPCType.Client);
            GetDayZularRPCManager().AddRPC("TraderX", "RedeemReceiptResponse", this, DayZularRPCType.Client);
        }
    }

    TraderXReceiptSettings GetSettings()
    {
        return m_Settings;
    }

    // ========================================================================
    // ITraderXReceiptProvider implementation
    // ========================================================================

    override bool CreateReceipt(PlayerBase player, string vehicleType, string presetId, ref array<string> presetAttachments, int buyPrice, ref array<int> lockData)
    {
        string uid = player.GetIdentity().GetPlainId();
        TraderXReceiptCollection collection = TraderXReceiptRepository.Load(uid);

        // Purge expired first
        collection.PurgeExpired();

        if (collection.GetPendingCount() >= m_Settings.maxReceiptsPerPlayer)
        {
            GetTraderXLogger().LogWarning("[Receipt] Max receipts reached for: " + uid);
            return false;
        }

        TraderXReceipt receipt = TraderXReceipt.Create(
            uid,
            vehicleType,
            presetId,
            presetAttachments,
            buyPrice,
            lockData,
            m_Settings.receiptExpiryDays
        );

        collection.AddReceipt(receipt);
        TraderXReceiptRepository.Save(uid, collection);

        // Spawn the physical paper item in player inventory
        SpawnReceiptItem(player, receipt);

        GetTraderXLogger().LogInfo(string.Format("[Receipt] Created: %1 for %2 (vehicle: %3)", receipt.receiptId, uid, vehicleType));
        return true;
    }

    override bool RedeemReceipt(PlayerBase player, string receiptId, int traderId, out string errorMsg)
    {
        errorMsg = "";
        string uid = player.GetIdentity().GetPlainId();
        TraderXReceiptCollection collection = TraderXReceiptRepository.Load(uid);
        TraderXReceipt receipt = collection.GetReceiptById(receiptId);

        if (!receipt)
        {
            errorMsg = "Receipt not found";
            return false;
        }

        if (!receipt.IsPending())
        {
            errorMsg = TernaryString(receipt.status == ETraderXReceiptStatus.REDEEMED, "Receipt already redeemed", "Receipt expired");
            return false;
        }

        if (receipt.IsExpired())
        {
            receipt.Expire();
            TraderXReceiptRepository.Save(uid, collection);
            errorMsg = "Receipt has expired";
            return false;
        }

        // Find parking spot
        string traderIdStr = traderId.ToString();
        TraderXVehicleParkingPosition parkingPos = TraderXVehicleParkingService.GetInstance().FindAvailableParkingPosition(traderIdStr);
        if (!parkingPos)
        {
            errorMsg = "No parking spot available at this trader";
            return false;
        }

        // ATOMIC: mark as redeemed BEFORE spawning (prevents duplicate redeem)
        receipt.Redeem();
        TraderXReceiptRepository.Save(uid, collection);

        // Build preset from receipt data
        TraderXPreset preset = null;
        if (receipt.presetId != string.Empty && receipt.presetAttachments && receipt.presetAttachments.Count() > 0)
        {
            preset = TraderXPreset.CreateTraderXPreset("Receipt", receipt.presetId, receipt.presetAttachments);
        }

        // Spawn vehicle
        EntityAI vehicle = TraderXVehicleFactory.SpawnVehicleWithPreset(
            receipt.vehicleType,
            preset,
            parkingPos.position,
            parkingPos.rotation
        );

        if (!vehicle)
        {
            // Rollback: un-redeem since spawn failed
            receipt.status = ETraderXReceiptStatus.PENDING;
            TraderXReceiptRepository.Save(uid, collection);
            errorMsg = "Failed to spawn vehicle";
            return false;
        }

        // Reserve parking
        TraderXVehicleParkingService.GetInstance().ReserveParkingPosition(traderIdStr, parkingPos, vehicle);

        // Restore lock data if available
        if (receipt.lockData && receipt.lockData.Count() >= 4)
        {
            ITraderXCarLockProvider lockProvider = TraderXProviderRegistry.GetInstance().GetCarLockProvider();
            if (lockProvider)
            {
                lockProvider.DeserializeLockData(vehicle, receipt.lockData);
            }
        }

        GetTraderXLogger().LogInfo(string.Format("[Receipt] Redeemed: %1 — spawned %2 at trader %3", receiptId, receipt.vehicleType, traderId));
        return true;
    }

    override bool IsReceiptValid(string receiptId, string playerUID)
    {
        TraderXReceiptCollection collection = TraderXReceiptRepository.Load(playerUID);
        TraderXReceipt receipt = collection.GetReceiptById(receiptId);
        return receipt && receipt.IsPending() && !receipt.IsExpired();
    }

    override int GetReceiptCount(string playerUID)
    {
        TraderXReceiptCollection collection = TraderXReceiptRepository.Load(playerUID);
        return collection.GetPendingCount();
    }

    override void PurgeExpiredReceipts(string playerUID)
    {
        TraderXReceiptCollection collection = TraderXReceiptRepository.Load(playerUID);
        int beforeCount = collection.GetPendingCount();
        collection.PurgeExpired();
        int afterCount = collection.GetPendingCount();
        if (beforeCount != afterCount)
        {
            TraderXReceiptRepository.Save(playerUID, collection);
            GetTraderXLogger().LogDebug(string.Format("[Receipt] Purged %1 expired receipts for: %2", beforeCount - afterCount, playerUID));
        }
    }

    // ========================================================================
    // Helpers
    // ========================================================================

    private void SpawnReceiptItem(PlayerBase player, TraderXReceipt receipt)
    {
        // Spawn the physical receipt paper in player inventory
        EntityAI receiptItem = player.GetInventory().CreateInInventory("TraderXReceiptItem");
        if (!receiptItem)
        {
            // Try to create near player if inventory is full
            receiptItem = EntityAI.Cast(GetGame().CreateObjectEx("TraderXReceiptItem", player.GetPosition(), ECE_PLACE_ON_SURFACE));
        }

        if (receiptItem)
        {
            // Set the receipt ID on the item so it can reference the DB record
            TraderXReceiptItem txReceipt = TraderXReceiptItem.Cast(receiptItem);
            if (txReceipt)
            {
                txReceipt.SetReceiptId(receipt.receiptId);
                txReceipt.SetVehicleType(receipt.vehicleType);
            }
        }
    }

    // ========================================================================
    // RPC Handlers
    // ========================================================================

    void GetPlayerReceiptsRequest(PlayerIdentity sender, ParamsReadContext ctx)
    {
        string uid = sender.GetPlainId();

        // Purge expired before sending
        PurgeExpiredReceipts(uid);

        TraderXReceiptCollection collection = TraderXReceiptRepository.Load(uid);
        GetDayZularRPCManager().SendRPC("TraderX", "GetPlayerReceiptsResponse", new Param1<ref array<ref TraderXReceipt>>(collection.receipts), true, sender);
    }

    void GetPlayerReceiptsResponse(PlayerIdentity sender, ParamsReadContext ctx)
    {
        Param1<ref array<ref TraderXReceipt>> data;
        if (!ctx.Read(data))
            return;

        Event_OnReceiptsReceived.Invoke(data.param1);
    }

    void RedeemReceiptRequest(PlayerIdentity sender, ParamsReadContext ctx)
    {
        Param2<string, int> data;
        if (!ctx.Read(data))
            return;

        PlayerBase player = TraderXHelper.GetPlayerByIdentity(sender);
        if (!player)
            return;

        string errorMsg;
        bool success = RedeemReceipt(player, data.param1, data.param2, errorMsg);

        string msg = TernaryString(success, "Vehicle deployed successfully!", errorMsg);
        GetDayZularRPCManager().SendRPC("TraderX", "RedeemReceiptResponse", new Param2<bool, string>(success, msg), true, sender);
    }

    void RedeemReceiptResponse(PlayerIdentity sender, ParamsReadContext ctx)
    {
        Param2<bool, string> data;
        if (!ctx.Read(data))
            return;

        Event_OnRedeemResult.Invoke(data.param1, data.param2);
    }
}
