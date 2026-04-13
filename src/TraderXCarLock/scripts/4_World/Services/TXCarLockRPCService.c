static ref TXCarLockRPCService g_TXCarLockRPCService;

TXCarLockRPCService GetTXCarLockRPCService()
{
    if (!g_TXCarLockRPCService)
        g_TXCarLockRPCService = TXCarLockRPCService.Cast(DZlrServiceRegistry.GetInstance().GetService(TXCarLockRPCService));
    return g_TXCarLockRPCService;
}

class TXCarLockRPCService : DayZularService
{
    void TXCarLockRPCService()
    {
        g_TXCarLockRPCService = this;

        GetDayZularEvents().OnPlayerJoined.Insert(OnPlayerJoined);

        RegisterRPCs();
    }

    void RegisterRPCs()
    {
        if (GetGame().IsClient())
        {
            GetDayZularRPCManager().AddRPC("TXCarLock", "GetCarLockConfigResponse", this, DayZularRPCType.Client);
            GetDayZularRPCManager().AddRPC("TXCarLock", "GetCarLockPlayerDbResponse", this, DayZularRPCType.Client);
            GetDayZularRPCManager().AddRPC("TXCarLock", "EnterPasswordAttemptResponse", this, DayZularRPCType.Client);
            GetDayZularRPCManager().AddRPC("TXCarLock", "GetLogLevelResponse", this, DayZularRPCType.Client);
        }

        if (GetGame().IsServer())
        {
            GetDayZularRPCManager().AddRPC("TXCarLock", "EnterPasswordAttempt", this, DayZularRPCType.Server);
            GetDayZularRPCManager().AddRPC("TXCarLock", "GetCarLockLockingRequest", this, DayZularRPCType.Server);
        }

        GetTXCarLockLogger().LogInfo("TXCarLockRPCService: RPCs registered");
    }

    void OnPlayerJoined(PlayerBase player, PlayerIdentity identity)
    {
        if (GetGame().IsServer())
        {
            TXCarLockPlayerDb playerDb = TXCarLockPlayerDb.Load(identity.GetPlainId(), identity.GetName());
            playerDb.SetAdmin(GetTXCarLockConfigService().IsCarLockAdmin(playerDb.GetOwnerId()));
            player.SetTXCarLockPlayerDb(playerDb);

            TXCarLockSettings settings = GetTXCarLockConfigService().GetSettings();
            GetDayZularRPCManager().SendRPC("TXCarLock", "GetCarLockConfigResponse", new Param1<TXCarLockSettings>(settings), true, identity);
            GetDayZularRPCManager().SendRPC("TXCarLock", "GetCarLockPlayerDbResponse", new Param1<TXCarLockPlayerDb>(playerDb), true, identity);

            GetTXCarLockLogger().LogInfo(string.Format("TXCarLockRPCService: Player joined - %1 (%2)", identity.GetName(), identity.GetPlainId()));
        }
    }

    // === Client RPC handlers ===

    void GetCarLockConfigResponse(PlayerIdentity sender, ParamsReadContext ctx)
    {
        Param1<TXCarLockSettings> data;
        if (!ctx.Read(data))
            return;

        GetTXCarLockConfigService().SetSettings(data.param1);
        GetTXCarLockLogger().LogDebug("TXCarLockRPCService: Config received from server");
    }

    void GetCarLockPlayerDbResponse(PlayerIdentity sender, ParamsReadContext ctx)
    {
        Param1<TXCarLockPlayerDb> data;
        if (!ctx.Read(data))
            return;

        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player)
            return;

        player.SetTXCarLockPlayerDb(data.param1);
        GetTXCarLockLogger().LogDebug("TXCarLockRPCService: PlayerDb received from server");
    }

    void EnterPasswordAttemptResponse(PlayerIdentity sender, ParamsReadContext ctx)
    {
        Param1<TXEnterPasswordResponse> data;
        if (!ctx.Read(data))
            return;

        GetTXCarLockUIService().OnEnterPasswordAttemptResponse(data.param1);
    }

    void GetLogLevelResponse(PlayerIdentity sender, ParamsReadContext ctx)
    {
        Param1<int> data;
        if (!ctx.Read(data))
            return;

        if (g_TXCarLockLogger)
            g_TXCarLockLogger.SetLogLevel(data.param1);
    }

    // === Server RPC handlers ===

    void EnterPasswordAttempt(PlayerIdentity sender, ParamsReadContext ctx)
    {
        Param2<Transport, int> data;
        if (!ctx.Read(data))
            return;

        GetTXCarLockLogger().LogDebug("TXCarLockRPCService: EnterPasswordAttempt");

        PlayerBase player = GetPlayerByIdentity(sender);
        if (!player)
            return;

        Transport entity = data.param1;
        if (!entity)
            return;

        VehicleLockManager manager = VehicleLockManager.Get(entity);
        if (!manager)
            return;

        int password = data.param2;

        TXCarLockPlayerDb playerDb = player.GetCarLockDb();
        if (!playerDb)
        {
            GetTXCarLockLogger().LogError("TXCarLockRPCService: PlayerDb not found");
            return;
        }

        bool isPasswordCorrect = (password == manager.GetVehicleLockPassword()) || playerDb.IsAdmin();

        if (!isPasswordCorrect)
        {
            GetDayZularRPCManager().SendRPC("TXCarLock", "EnterPasswordAttemptResponse", new Param1<TXEnterPasswordResponse>(TXEnterPasswordResponse.Create("Code incorrect", false)), true, sender);
            return;
        }

        if (!playerDb.HasUnlockedVehicle(manager.GetLockId()))
        {
            playerDb.AddUnlockedVehicle(manager.GetLockId());
        }

        manager.SetVehicleLock(false);
        GetDayZularRPCManager().SendRPC("TXCarLock", "EnterPasswordAttemptResponse", new Param1<TXEnterPasswordResponse>(TXEnterPasswordResponse.Create("Code Valid", true)), true, sender);
        GetDayZularRPCManager().SendRPC("TXCarLock", "GetCarLockPlayerDbResponse", new Param1<TXCarLockPlayerDb>(player.GetCarLockDb()), true, sender);
    }

    void GetCarLockLockingRequest(PlayerIdentity sender, ParamsReadContext ctx)
    {
        Param3<Transport, int, bool> data;
        if (!ctx.Read(data))
            return;

        GetTXCarLockLogger().LogDebug("TXCarLockRPCService: GetCarLockLockingRequest");

        PlayerBase player = GetPlayerByIdentity(sender);
        if (!player)
            return;

        Transport entity = data.param1;
        if (!entity)
            return;

        VehicleLockManager manager = VehicleLockManager.Get(entity);
        if (!manager)
            return;

        int password = data.param2;
        bool lock = data.param3;

        if (!manager.HasVehicleLockPassword())
        {
            manager.InitializeVehicleLockData(player.GetCarLockDb().GetOwnerId(), password);
        }
        else
        {
            manager.SetVehicleLock(lock);
        }

        player.GetCarLockDb().AddUnlockedVehicle(manager.GetLockId());
        GetDayZularRPCManager().SendRPC("TXCarLock", "GetCarLockPlayerDbResponse", new Param1<TXCarLockPlayerDb>(player.GetCarLockDb()), true, sender);
    }

    // === Utility ===

    static PlayerBase GetPlayerByIdentity(PlayerIdentity identity)
    {
        int low = 0;
        int high = 0;
        GetGame().GetPlayerNetworkIDByIdentityID(identity.GetPlayerId(), low, high);
        return PlayerBase.Cast(GetGame().GetObjectByNetworkId(low, high));
    }
}
