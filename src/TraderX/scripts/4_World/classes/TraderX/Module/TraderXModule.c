static ref TraderXModule g_TraderXModule;

static ref TraderXModule GetTraderXModule()
{
    if (!g_TraderXModule)
        g_TraderXModule = TraderXModule.Cast(DZlrServiceRegistry.GetInstance().GetService(TraderXModule));
    return g_TraderXModule;
}

class TraderXModule : DayZularService
{
    ref TraderXGeneralSettings generalSettings;

    static ref ScriptInvoker Event_OnTraderXPlayerJoined = new ScriptInvoker();

    private float m_DeltaAccumulator;

    void TraderXModule()
    {
        g_TraderXModule = this;

        // Subscribe to DayZular lifecycle events
        GetDayZularEvents().OnMissionStart.Insert(OnMissionStart);
        GetDayZularEvents().OnUpdate.Insert(OnUpdate);
    }

    void InitializeModule()
    {
        if (GetGame().IsServer())
        {
            // Convert TraderPlus V1 configs if detected (runs before anything else)
            ConvertTraderPlus.TryConvert();

            // Convert old TraderX V1 internal format to V2 (legacy migration)
            oldTraderXPriceConfig.ConvertOldConfigToNew();
            oldTraderXGeneralSettings.ConvertOldConfigToNew();

            // Load user's source format preference
            TraderXSourceConfig sourceConfig = TraderXSourceConfigLoader.LoadConfig();

            // Initialize configuration using the service (handles all format loading and migration)
            TraderXConfigurationService.GetInstance().Initialize(sourceConfig);

            generalSettings = TraderXSettingsRepository.Load();
            TraderXPresetsService.GetInstance().GetInstance();
            TraderXNpcService.GetInstance().CreateNpcs();

            // Initialize insurance and receipt services
            TraderXInsuranceService.GetInstance().Initialize();
            TraderXReceiptService.GetInstance().Initialize();
        }
        else
        {
            generalSettings = new TraderXGeneralSettings();
            TraderXTransactionNotifier.GetInstance();
            TraderXNotificationService.GetInstance();
            TraderXPresetsService.GetInstance().GetInstance();
        }

        InitRPCs();

        // Schedule insurance scan for missing vehicles (server only, delayed)
        if (GetGame().IsServer() && TraderXProviderRegistry.GetInstance().HasInsuranceProvider())
        {
            GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(TraderXInsuranceService.GetInstance().ScanForMissingVehicles, 5000);
        }
    }

    TraderXGeneralSettings GetSettings()
    {
        return generalSettings;
    }

    void InitRPCs()
    {
        TraderXTransactionService.GetInstance().RegisterRPCs();
        TraderXPresetsService.GetInstance().RegisterRPCs();
        TraderXCurrencyService.GetInstance().RegisterRPCs();
        TraderXVehicleParkingService.GetInstance().RegisterRPCs();
        TraderXInsuranceService.GetInstance().RegisterRPCs();
        TraderXReceiptService.GetInstance().RegisterRPCs();

        if (GetGame().IsClient())
        {
            GetDayZularRPCManager().AddRPC("TraderX", "GetConfigResponse", this, DayZularRPCType.Client);
            GetDayZularRPCManager().AddRPC("TraderX", "GetTraderCategoriesResponse", this, DayZularRPCType.Client);
            GetDayZularRPCManager().AddRPC("TraderX", "GetTraderProductsResponse", this, DayZularRPCType.Client);
            GetDayZularRPCManager().AddRPC("TraderX", "GetTraderStockResponse", this, DayZularRPCType.Client);
            GetDayZularRPCManager().AddRPC("TraderX", "GetPlayerLicensesResponse", this, DayZularRPCType.Client);
            GetDayZularRPCManager().AddRPC("TraderX", "GetTraderXCurrencyResponse", TraderXCurrencyService.GetInstance(), DayZularRPCType.Client);
        }
        else
        {
            GetDayZularRPCManager().AddRPC("TraderX", "OnTraderXMenuOpen", this, DayZularRPCType.Server);
            GetDayZularRPCManager().AddRPC("TraderX", "OnTraderXMenuClose", this, DayZularRPCType.Server);
        }
    }

    void OpenTraderView()
    {
        TraderXUIService.GetInstance().OpenTraderView();
    }

    bool IsTraderViewOpen()
    {
        return TraderXUIService.GetInstance().IsTraderViewOpen();
    }

    void CloseTraderView()
    {
        TraderXUIService.GetInstance().CloseTraderView();
    }

    void OnTraderXMenuOpen(PlayerIdentity sender, ParamsReadContext ctx)
    {
        PlayerBase player = TraderXHelper.GetPlayerByIdentity(sender);
        if (!player)
            return;

        Param1<int> data;
        if (!ctx.Read(data))
            return;

        int npcId = data.param1;
        TraderXNpcSessionService.GetInstance().AddPlayerToNpcId(player, npcId);
        SendTraderStockToClient(player, npcId);
    }

    void OnTraderXMenuClose(PlayerIdentity sender, ParamsReadContext ctx)
    {
        PlayerBase player = TraderXHelper.GetPlayerByIdentity(sender);
        if (!player)
            return;

        Param1<int> data;
        if (!ctx.Read(data))
            return;

        int npcId = data.param1;
        TraderXNpcSessionService.GetInstance().RemovePlayerFromNpcId(player, npcId);
    }

    void SendTraderStockToClient(PlayerBase player, int npcId)
    {
        TraderXNpc npc = generalSettings.GetNpcById(npcId);

        if (!npc)
        {
            GetTraderXLogger().LogError("GetTraderStockRequest:: npc wasn't found from id: " + npcId);
            return;
        }

        array<ref TraderXProductStock> npcStock = new array<ref TraderXProductStock>();
        TraderXProductStockRepository.GetStockArrForCategoriesId(npc.categoriesId, npcStock);
        GetDayZularRPCManager().SendRPC("TraderX", "GetTraderStockResponse", new Param1<array<ref TraderXProductStock>>(npcStock), true, player.GetIdentity());
    }

    void GetTraderStockResponse(PlayerIdentity sender, ParamsReadContext ctx)
    {
        Param1<array<ref TraderXProductStock>> data;
        if (!ctx.Read(data))
            return;

        array<ref TraderXProductStock> npcStock = data.param1;
        TraderXProductStockRepository.RefStockToTraderXProduct(npcStock);
        TraderXTradingService.GetInstance().OnTraderXResponseReceived(ETraderXResponse.ALL_STOCK_RECEIVED);
    }

    void GetPlayerLicensesResponse(PlayerIdentity sender, ParamsReadContext ctx)
    {
        Param1<TraderXPlayerLicenses> data;
        if (!ctx.Read(data))
            return;

        TraderXLicenseService.GetInstance().OnPlayerLicensesResponse(data.param1);
    }

    void GetConfigResponse(PlayerIdentity sender, ParamsReadContext ctx)
    {
        Param1<TraderXGeneralSettings> data;
        if (!ctx.Read(data))
            return;

        GetTraderXLogger().LogDebug("GetConfigResponse serverId " + data.param1.serverID);

        generalSettings = data.param1;
        TraderXInventoryManager.CheckEntityNetworkId(PlayerBase.Cast(GetGame().GetPlayer()));

        TraderXPresetsService.GetInstance().SetServerId(data.param1.serverID);
    }

    void GetTraderCategoriesResponse(PlayerIdentity sender, ParamsReadContext ctx)
    {
        Param1<array<ref TraderXCategory>> data;
        if (!ctx.Read(data))
        {
            GetTraderXLogger().LogError("GetTraderCategoriesResponse:: failed to read data");
            return;
        }

        GetTraderXLogger().LogDebug("GetTraderCategoriesResponse:: data read");

        TraderXCategoryRepository.SetCategories(data.param1);
        TraderXCategoryRepository.DebugSaveAllCategories();
    }

    void GetTraderProductsResponse(PlayerIdentity sender, ParamsReadContext ctx)
    {
        Param1<array<ref TraderXProduct>> data;
        if (!ctx.Read(data))
        {
            GetTraderXLogger().LogError("GetTraderProductsResponse:: failed to read data");
            return;
        }

        TraderXJsonLoader<array<ref TraderXProduct>>.SaveToFile(TRADERX_PRODUCTS_DIR + "RPCAllItems.json", data.param1);
        TraderXProductRepository.SetProducts(data.param1);
        TraderXProductRepository.DebugSaveAllItems();
    }

    void SendGeneralConfig(PlayerIdentity identity)
    {
        GetDayZularRPCManager().SendRPC("TraderX", "GetConfigResponse", new Param1<TraderXGeneralSettings>(generalSettings), true, identity);
    }

    void SendTraderXCategories(PlayerIdentity identity)
    {
        GetDayZularRPCManager().SendRPC("TraderX", "GetTraderCategoriesResponse", new Param1<array<ref TraderXCategory>>(TraderXCategoryRepository.GetCategories()), true, identity);
    }

    void SendTraderXProducts(PlayerIdentity identity)
    {
        TraderXJsonLoader<array<ref TraderXProduct>>.SaveToFile(TRADERX_PRODUCTS_DIR + "RPCAllItems.json", TraderXProductRepository.GetProducts());
        GetDayZularRPCManager().SendRPC("TraderX", "GetTraderProductsResponse", new Param1<array<ref TraderXProduct>>(TraderXProductRepository.GetProducts()), true, identity);
    }

    void OnPlayerJoined(PlayerBase player, PlayerIdentity identity)
    {
        SendGeneralConfig(identity);
        SendTraderXCategories(identity);
        SendTraderXProducts(identity);
        TraderXInventoryManager.CheckEntityNetworkId(player);
        Event_OnTraderXPlayerJoined.Invoke(player, identity);
    }

    void OnMissionStart()
    {
        if (GetGame().IsServer())
        {
            GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(InitializeModule, 1000);
        }
        else
        {
            InitializeModule();
        }
    }

    void OnUpdate(float dt)
    {
        if (GetGame().IsServer())
        {
            TraderXTransactionService.GetInstance().ProcessTransactionQueue(dt);
        }
        else if (GetGame().IsClient())
        {
            TraderXTransactionService.GetInstance().ProcessTransactionResponseQueue(dt);
        }
    }
}
