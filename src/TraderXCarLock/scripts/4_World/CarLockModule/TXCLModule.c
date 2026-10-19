#ifndef CarLock
// TraderPlus V1 defines "CarLock" and owns the CarLock item: TraderX CarLock stands down when it is loaded (see TXCLTraderPlusNotice).
static TXCLModule GetTXCLModule()
{
    return TXCLModule.Cast(CF_ModuleCoreManager.Get(TXCLModule));
}

[CF_RegisterModule(TXCLModule)]
class TXCLModule : CF_ModuleWorld
{
    ref TXCLSettings settings;

    // Client-only menu state.
    ref ComponentMenu m_LockMenu;
    ref TXCLLockMenuComponent m_MenuComponent;
    ref TXCLCooldownState m_LastCooldownState;

    override void OnInit()
    {
        super.OnInit();
        EnableInvokeConnect();
        EnableMissionStart();

        // Decide the vehicle-storage mode before the world (and its vehicles) is loaded.
        if (GetGame().IsDedicatedServer())
            TXCLStorageMode.Resolve();
    }

    override void OnMissionStart(Class sender, CF_EventArgs args)
    {
        super.OnMissionStart(sender, args);
        GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(InitializeModule, 1000);
    }

    void InitializeModule()
    {
        if (GetGame().IsServer())
        {
            settings = TXCLSettings.Load();
            SetCarLockClassNames();
            InitRPCserver();
            GetTXCLLog().LogInfo("[TXCL] Module initialized (server), passwordLength=" + settings.passwordLength);

            // Expose the carlock to other TraderX mods. Server-side only.
#ifdef TraderX
            TraderXProviderRegistry.GetInstance().RegisterCarLockProvider(new TXCLLockProvider());
#endif
        }

        if (GetGame().IsClient())
        {
            if (!settings)
                settings = new TXCLSettings();
            InitRPCclient();
            GetTXCLLog().LogInfo("[TXCL] Module initialized (client)");
        }
    }

    TXCLSettings GetSettings()
    {
        return settings;
    }

    void SetCarLockClassNames()
    {
        string defaultLockpick = "CarLockPick";
        if (settings && settings.lockpickingSettings && settings.lockpickingSettings.carConfig)
            defaultLockpick = settings.lockpickingSettings.carConfig.lockpickClassName;
        ItemBase.SetTXCLClassNames(settings.carLockClassName, defaultLockpick);
    }

    // ---- RPC registration ---------------------------------------------------

    void InitRPCserver()
    {
        GetRPCManager().AddRPC(TXCL_RPC_NAMESPACE, "GetCooldownState", this, SingeplayerExecutionType.Server);
        GetRPCManager().AddRPC(TXCL_RPC_NAMESPACE, "EnterPasswordAttempt", this, SingeplayerExecutionType.Server);
        GetRPCManager().AddRPC(TXCL_RPC_NAMESPACE, "LockingRequest", this, SingeplayerExecutionType.Server);
    }

    void InitRPCclient()
    {
        GetRPCManager().AddRPC(TXCL_RPC_NAMESPACE, "GetCooldownStateResponse", this, SingeplayerExecutionType.Client);
        GetRPCManager().AddRPC(TXCL_RPC_NAMESPACE, "EnterPasswordAttemptResponse", this, SingeplayerExecutionType.Client);
        GetRPCManager().AddRPC(TXCL_RPC_NAMESPACE, "GetConfigResponse", this, SingeplayerExecutionType.Client);
        GetRPCManager().AddRPC(TXCL_RPC_NAMESPACE, "GetPlayerDbResponse", this, SingeplayerExecutionType.Client);
    }

    // ---- Client menu --------------------------------------------------------

    void OpenLockMenu(TXCLVehicleLockManager manager)
    {
        if (!manager)
            return;

        if (m_LockMenu && m_LockMenu.IsVisible())
            return;

        m_LockMenu = new ComponentMenu();
        m_LockMenu.SetUseMouse(true);
        m_LockMenu.SetUseKeyboard(true);

        m_MenuComponent = new TXCLLockMenuComponent();
        m_MenuComponent.SetManager(manager);
        m_MenuComponent.InitComponent(null);

        m_LockMenu.SetRootComponent(m_MenuComponent);
        m_LockMenu.ShowMenu();

        m_MenuComponent.OnMenuShown();

        if (m_LastCooldownState)
            m_MenuComponent.SetCooldownState(m_LastCooldownState);
    }

    void CloseLockMenu()
    {
        if (!m_LockMenu)
            return;

        m_LockMenu.HideMenu();
        m_MenuComponent = null;
        m_LockMenu = null;
    }

    // ---- Client RPC handlers ------------------------------------------------

    void GetCooldownStateResponse(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type != CallType.Client)
            return;

        Param1<TXCLCooldownState> data;
        if (!ctx.Read(data))
            return;

        m_LastCooldownState = data.param1;
        if (m_MenuComponent)
            m_MenuComponent.SetCooldownState(data.param1);
    }

    void EnterPasswordAttemptResponse(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type != CallType.Client)
            return;

        Param1<TXCLEnterPasswordResponse> data;
        if (!ctx.Read(data))
            return;

        if (m_MenuComponent)
            m_MenuComponent.OnEnterPasswordResponse(data.param1);
    }

    void GetConfigResponse(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type != CallType.Client)
            return;

        Param1<TXCLSettings> data;
        if (!ctx.Read(data))
            return;

        settings = data.param1;
        SetCarLockClassNames();
    }

    void GetPlayerDbResponse(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type != CallType.Client)
            return;

        Param1<TXCLPlayerDb> data;
        if (!ctx.Read(data))
            return;

        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player)
            return;

        player.SetTXCLPlayerDb(data.param1);
    }

    // ---- Server lifecycle + RPC handlers ------------------------------------

    override void OnInvokeConnect(Class sender, CF_EventArgs args)
    {
        super.OnInvokeConnect(sender, args);

        if (!IsMissionHost())
            return;

        CF_EventPlayerArgs cArgs = CF_EventPlayerArgs.Cast(args);
        if (!cArgs || !cArgs.Player || !cArgs.Identity)
            return;

        OnPlayerConnect(cArgs.Player, cArgs.Identity);
    }

    void OnPlayerConnect(PlayerBase player, PlayerIdentity identity)
    {
        TXCLPlayerDb playerDb = TXCLPlayerDb.Load(identity.GetPlainId(), identity.GetName());
        playerDb.SetAdmin(IsCarLockAdmin(playerDb.GetOwnerId()));
        player.SetTXCLPlayerDb(playerDb);
        GetTXCLLog().LogInfo("[TXCL] Player connected: " + identity.GetName() + " admin=" + playerDb.IsAdmin());

        GetRPCManager().SendRPC(TXCL_RPC_NAMESPACE, "GetConfigResponse", new Param1<TXCLSettings>(settings), true, identity);
        GetRPCManager().SendRPC(TXCL_RPC_NAMESPACE, "GetPlayerDbResponse", new Param1<TXCLPlayerDb>(playerDb), true, identity);
    }

    bool IsCarLockAdmin(int id)
    {
        if (!settings || !settings.admins)
            return false;
        foreach (string steamId : settings.admins)
        {
            if (steamId.Contains(id.ToString()))
                return true;
        }
        return false;
    }

    void GetCooldownState(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type != CallType.Server)
            return;

        Param1<Transport> data;
        if (!ctx.Read(data))
            return;

        PlayerBase player = GetPlayerByIdentity(sender);
        if (!player)
            return;

        Transport entity = data.param1;
        if (!entity)
            return;

        TXCLVehicleLockManager manager = TXCLVehicleLockManager.Get(entity);
        if (!manager)
            return;

        TXCLPlayerDb playerDb = player.GetTXCLPlayerDb();
        if (!playerDb)
            return;

        TXCLCooldownState cooldownState = playerDb.GetCooldownStateById(manager.GetLockId());
        if (cooldownState && cooldownState.HasCooldown())
        {
            cooldownState.PrepareForClient();
            GetRPCManager().SendRPC(TXCL_RPC_NAMESPACE, "GetCooldownStateResponse", new Param1<TXCLCooldownState>(cooldownState), true, player.GetIdentity());
        }
    }

    void EnterPasswordAttempt(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type != CallType.Server)
            return;

        Param2<Transport, int> data;
        if (!ctx.Read(data))
            return;

        PlayerBase player = GetPlayerByIdentity(sender);
        if (!player)
            return;

        Transport entity = data.param1;
        if (!entity)
            return;

        TXCLVehicleLockManager manager = TXCLVehicleLockManager.Get(entity);
        if (!manager)
            return;

        TXCLAttemptService.GetInstance().EnterPasswordAttempt(manager, player, data.param2);
    }

    void LockingRequest(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type != CallType.Server)
            return;

        Param3<Transport, int, bool> data;
        if (!ctx.Read(data))
            return;

        PlayerBase player = GetPlayerByIdentity(sender);
        if (!player)
            return;

        Transport entity = data.param1;
        if (!entity)
            return;

        TXCLVehicleLockManager manager = TXCLVehicleLockManager.Get(entity);
        if (!manager)
            return;

        TXCLPlayerDb playerDb = player.GetTXCLPlayerDb();
        if (!playerDb)
            return;

        int password = data.param2;
        bool lock = data.param3;

        if (!manager.HasVehicleLockPassword())
        {
            manager.InitializeVehicleLockData(playerDb.GetOwnerId(), password);
            GetTXCLLog().LogInfo("[TXCL] Password set by owner " + playerDb.GetOwnerId() + " lockId=" + manager.GetLockId());
        }
        else
        {
            manager.SetVehicleLock(lock);
            GetTXCLLog().LogInfo("[TXCL] Lock toggled=" + lock + " lockId=" + manager.GetLockId());
        }

        playerDb.AddUnlockedVehicle(manager.GetLockId());
        GetRPCManager().SendRPC(TXCL_RPC_NAMESPACE, "GetPlayerDbResponse", new Param1<TXCLPlayerDb>(playerDb), true, sender);
    }

    static PlayerBase GetPlayerByIdentity(PlayerIdentity identity)
    {
        if (!identity)
            return null;
        int low = 0;
        int high = 0;
        GetGame().GetPlayerNetworkIDByIdentityID(identity.GetPlayerId(), low, high);
        return PlayerBase.Cast(GetGame().GetObjectByNetworkId(low, high));
    }
}
#endif
