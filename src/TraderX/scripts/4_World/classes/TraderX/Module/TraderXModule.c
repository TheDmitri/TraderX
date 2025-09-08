static ref TraderXModule GetTraderXModule()
{
    return TraderXModule.Cast(CF_ModuleCoreManager.Get(TraderXModule));
}

[CF_RegisterModule(TraderXModule)]
class TraderXModule : CF_ModuleWorld
{
    ref TraderXGeneralSettings generalSettings;

    static ref ScriptInvoker Event_OnTraderXPlayerJoined = new ScriptInvoker();

    ref TraderXMainView traderXMainView;

    override void OnInit()
    {
        super.OnInit();
        EnableMissionStart();
        EnableUpdate();
    }

    void InitializeModule()
    {
        if(GetGame().IsServer())
        {
            // Convert old V1 configs to V2 format before loading repositories
            oldTraderXPriceConfig.ConvertOldConfigToNew();
            oldTraderXGeneralSettings.ConvertOldConfigToNew();
            
            TraderXProductRepository.LoadAllProducts();
            TraderXCategoryRepository.LoadAllCategories();
            TraderXVehicleParkingRepository.LoadAllParkingCollections();

            generalSettings = TraderXSettingsRepository.Load();
            TraderXPresetsService.GetInstance().GetInstance();
            TraderXNpcService.GetInstance().CreateNpcs();
        }
        else
        {
            generalSettings = new TraderXGeneralSettings();
            TraderXTransactionNotifier.GetInstance();
            TraderXNotificationService.GetInstance();
            TraderXPresetsService.GetInstance().GetInstance();
        }

        InitRPCs();
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

        if(GetGame().IsClient())
        {
            GetRPCManager().AddRPC("TraderX", "GetConfigResponse", this, SingeplayerExecutionType.Client);
            GetRPCManager().AddRPC("TraderX", "GetTraderCategoriesResponse", this, SingeplayerExecutionType.Client);
            GetRPCManager().AddRPC("TraderX", "GetTraderProductsResponse", this, SingeplayerExecutionType.Client);
            GetRPCManager().AddRPC("TraderX", "GetTraderStockResponse", this, SingeplayerExecutionType.Client);
            GetRPCManager().AddRPC("TraderX", "GetPlayerLicensesResponse", this, SingeplayerExecutionType.Client);
            GetRPCManager().AddRPC("TraderX", "GetTraderXCurrencyResponse", TraderXCurrencyService.GetInstance(), SingeplayerExecutionType.Client);
        }
        else
        {
            GetRPCManager().AddRPC("TraderX", "OnTraderXMenuOpen", this, SingeplayerExecutionType.Server);
            GetRPCManager().AddRPC("TraderX", "OnTraderXMenuClose", this, SingeplayerExecutionType.Server);
        }
    }

    void OpenTraderView()
    {
        if(traderXMainView){
            traderXMainView.Show(true);
            return;
        }

        traderXMainView = new TraderXMainView();
        traderXMainView.Show(true);
    }

    bool IsTraderViewOpen()
    {
        return traderXMainView != null;
    }

    void CloseTraderView()
    {
        if(traderXMainView){
            TraderXTradingService.GetInstance().ClearNpc();
            traderXMainView.Show(false);
            delete traderXMainView;
        }
    }

    void OnTraderXMenuOpen(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if(type != CallType.Server)
            return;

        PlayerBase player = TraderXHelper.GetPlayerByIdentity(sender);
        if(!player)
            return;

        Param1<int> data;
        if(!ctx.Read(data)){
            return;
        }

        int npcId = data.param1;
        TraderXNpcSessionService.GetInstance().AddPlayerToNpcId(player, npcId);
        SendTraderStockToClient(player, npcId);
    }

    void OnTraderXMenuClose(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if(type != CallType.Server)
            return;

        PlayerBase player = TraderXHelper.GetPlayerByIdentity(sender);
        if(!player)
            return;

        Param1<int> data;
        if(!ctx.Read(data)){
            return;
        }

        int npcId = data.param1;
        TraderXNpcSessionService.GetInstance().RemovePlayerFromNpcId(player, npcId);
    }

    void SendTraderStockToClient(PlayerBase player, int npcId)
    {
        TraderXNpc npc = generalSettings.GetNpcById(npcId);

        if(!npc){
            GetTraderXLogger().LogError("GetTraderStockRequest:: npc wasn't found from id: " + npcId);
            return;
        }

        array<ref TraderXProductStock> npcStock = new array<ref TraderXProductStock>();
        TraderXProductStockRepository.GetStockArrForCategoriesId(npc.categoriesId, npcStock);
        GetRPCManager().SendRPC("TraderX", "GetTraderStockResponse", new Param1<array<ref TraderXProductStock>>(npcStock), true, player.GetIdentity());
    }

    void GetTraderStockResponse(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if(type != CallType.Client)
            return;

        Param1<array<ref TraderXProductStock>> data;
        if(!ctx.Read(data)){
            return;
        }

        array<ref TraderXProductStock> npcStock = data.param1;
        TraderXProductStockRepository.RefStockToTraderXProduct(npcStock);
        TraderXTradingService.GetInstance().OnTraderXResponseReceived(ETraderXResponse.ALL_STOCK_RECEIVED);
    }

    void GetPlayerLicensesResponse(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if(type != CallType.Client)
            return;

        Param1<TraderXPlayerLicenses> data;
        if(!ctx.Read(data)){
            return;
        }

        TraderXLicenseService.GetInstance().OnPlayerLicensesResponse(data.param1);
    }

    void GetConfigResponse(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if(type != CallType.Client)
            return;

        Param1<TraderXGeneralSettings> data;
        if(!ctx.Read(data)){
            return;
        }

        GetTraderXLogger().LogDebug("GetConfigResponse serverId " + data.param1.serverID);

        generalSettings = data.param1;
        TraderXInventoryManager.CheckEntityNetworkId(PlayerBase.Cast(GetGame().GetPlayer()));

        TraderXPresetsService.GetInstance().SetServerId(data.param1.serverID);
    }

    void GetTraderCategoriesResponse(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if(type != CallType.Client)
            return;

        Param1<array<ref TraderXCategory>> data;
        if(!ctx.Read(data)){
            GetTraderXLogger().LogError("GetTraderCategoriesResponse:: failed to read data");
            return;
        }

        GetTraderXLogger().LogDebug("GetTraderCategoriesResponse:: data read");

        TraderXCategoryRepository.SetCategories(data.param1);
        TraderXCategoryRepository.DebugSaveAllCategories();
    }

    void GetTraderProductsResponse(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if(type != CallType.Client)
            return;

        Param1<array<ref TraderXProduct>> data;
        if(!ctx.Read(data)){
            GetTraderXLogger().LogError("GetTraderProductsResponse:: failed to read data");
            return;
        }

        TraderXJsonLoader<array<ref TraderXProduct>>.SaveToFile(TRADERX_PRODUCTS_DIR + "RPCAllItems.json", data.param1);
        TraderXProductRepository.SetProducts(data.param1);
        TraderXProductRepository.DebugSaveAllItems();
    }

    void SendGeneralConfig(PlayerIdentity identity)
    {
        GetRPCManager().SendRPC("TraderX", "GetConfigResponse", new Param1<TraderXGeneralSettings>(generalSettings), true, identity);
    }

    void SendTraderXCategories(PlayerIdentity identity)
    {
        GetRPCManager().SendRPC("TraderX", "GetTraderCategoriesResponse", new Param1<array<ref TraderXCategory>>(TraderXCategoryRepository.GetCategories()), true, identity);
    }

    void SendTraderXProducts(PlayerIdentity identity)
    {
        TraderXJsonLoader<array<ref TraderXProduct>>.SaveToFile(TRADERX_PRODUCTS_DIR + "RPCAllItems.json", TraderXProductRepository.GetProducts());
        GetRPCManager().SendRPC("TraderX", "GetTraderProductsResponse", new Param1<array<ref TraderXProduct>>(TraderXProductRepository.GetProducts()), true, identity);
    }

    void OnPlayerJoined(PlayerBase player, PlayerIdentity identity)
    {
        SendGeneralConfig(identity);
        SendTraderXCategories(identity);
        SendTraderXProducts(identity);
        TraderXInventoryManager.CheckEntityNetworkId(player);
        Event_OnTraderXPlayerJoined.Invoke(player, identity);
    }

    override void OnMissionStart(Class sender, CF_EventArgs args)
    {
        super.OnMissionStart(sender, args);
        if(GetGame().IsServer()){
            GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(InitializeModule, 1000);
        }
        else{
            InitializeModule();
        }
    }

    override void OnUpdate(Class sender, CF_EventArgs args)
    {
        super.OnUpdate(sender, args);
        auto update = CF_EventUpdateArgs.Cast(args);
    
        if (GetGame().IsServer())
        {
            TraderXTransactionService.GetInstance().ProcessTransactionQueue(update.DeltaTime);
        }
        else if(GetGame().IsClient())
        {
           TraderXTransactionService.GetInstance().ProcessTransactionResponseQueue(update.DeltaTime);
        }
    }
}