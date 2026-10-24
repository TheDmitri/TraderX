static ref TraderXModule GetTraderXModule()
{
    return TraderXModule.Cast(CF_ModuleCoreManager.Get(TraderXModule));
}

// Per-player timing for the safezone catalog prewarm (debug logging only).
class TraderXPrewarmTiming
{
    float startTimeMs;
    int pendingNpcs;
    int totalProducts;
}

[CF_RegisterModule(TraderXModule)]
class TraderXModule : CF_ModuleWorld
{
    ref TraderXGeneralSettings generalSettings;

    static ref ScriptInvoker Event_OnTraderXPlayerJoined = new ScriptInvoker();

    // Active safezone prewarm streams, keyed by player steamId — for the debug duration log.
    ref map<string, ref TraderXPrewarmTiming> m_PrewarmTiming = new map<string, ref TraderXPrewarmTiming>();


    override void OnInit()
    {
        super.OnInit();
        EnableMissionStart();
        EnableUpdate();

        // Decide how to read vehicles saved without TraderX before the world (and its vehicles) is loaded
        if (GetGame().IsDedicatedServer())
            TraderXWorldFormat.Resolve();
    }

    void InitializeModule()
    {
        if(GetGame().IsServer())
        {
            // A new server has no general settings yet (checked before the importers create them)
            bool freshInstall = !FileExist(TRADERX_CONFIG_FILE);
            bool importedThisBoot = false;

            // Import TraderPlus V1 configs into an empty TraderX install (never over an existing one)
            if (TraderXLegacyConfigPath.ShouldImport())
            {
                TraderXLegacyConfigPath.EnsureTraderXDirs();
                oldTraderXPriceConfig.ConvertOldConfigToNew();
                oldTraderXGeneralSettings.ConvertOldConfigToNew();
                oldTraderPlusVehiclesConfig.ConvertOldConfigToNew();
                TraderXLegacyConfigPath.MarkImported();
                TraderXMigrationReport.Write("TraderPlus");
                importedThisBoot = true;
            }

            // Import catalogs from other trader mods (one-shot, sentinel-guarded, non-destructive).
            bool drJonesDone = TraderXExternalMigration.IsDone("drjones");
            bool expansionDone = TraderXExternalMigration.IsDone("expansion");
            ConvertDrJonesTrader.ConvertIfPresent();
            ConvertExpansionMarket.ConvertIfPresent();
            if ((!drJonesDone && TraderXExternalMigration.IsDone("drjones")) || (!expansionDone && TraderXExternalMigration.IsDone("expansion")))
                importedThisBoot = true;

            // Load user's source format preference (a new server starts on CSV)
            TraderXSourceConfig sourceConfig = TraderXSourceConfigLoader.LoadConfig(freshInstall);

            // Initialize configuration using the service (handles all format loading and migration)
            TraderXConfigurationService.GetInstance().Initialize(sourceConfig, importedThisBoot);

            generalSettings = TraderXSettingsRepository.Load();
            TraderXPresetsService.GetInstance().GetInstance();
            TraderXNpcService.GetInstance().CreateNpcs();
        }
        else
        {
            generalSettings = new TraderXGeneralSettings();
            TraderXTransactionNotifier.GetInstance();
            // Legacy TraderXNotificationService stack kept dormant — toast routing
            // now goes through TraderXToastService (registered in TraderXAppInit).
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
        if (GetTraderXUIService())
            GetTraderXUIService().OpenTraderView();
    }

    bool IsTraderViewOpen()
    {
        if (GetTraderXUIService())
            return GetTraderXUIService().IsTraderViewOpen();
        return false;
    }

    void CloseTraderView()
    {
        if (GetTraderXUIService())
            GetTraderXUIService().CloseTraderView();
    }

    void OnTraderXMenuOpen(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if(type != CallType.Server)
            return;

        PlayerBase player = TraderXHelper.GetPlayerByIdentity(sender);
        if(!player)
            return;

        // param2 = needProducts: false when the client already has this trader's products
        // (e.g. prewarmed on safezone entry) — then we skip the product stream and send stock only.
        Param2<int, bool> data = new Param2<int, bool>(0, true);
        if(!ctx.Read(data)){
            return;
        }

        int npcId = data.param1;
        bool needProducts = data.param2;
        TraderXNpcSessionService.GetInstance().AddPlayerToNpcId(player, npcId);

        if (needProducts)
        {
            // Stream this trader's product subset first, then stock. Stock is sent from the LAST
            // product chunk so — over the ordered guaranteed channel — all products land on the
            // client before the stock arrives (the client's render gate waits on stock).
            SendNpcProductChunk(sender, npcId, 0, true);
        }
        else
        {
            // Products already cached client-side — stock only, for an instant open.
            SendTraderStockToClient(player, npcId);
        }
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

        GetTraderXLogger().LogDebug("GetConfigResponse serverId " + data.param1.serverID + " traders=" + data.param1.traders.Count());

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

        GetTraderXLogger().LogDebug("GetTraderCategoriesResponse:: data read count=" + data.param1.Count());
        if (data.param1.Count() > 0 && data.param1[0])
        {
            TraderXCategory firstCat = data.param1[0];
            int pidCount = 0;
            if (firstCat.productIds) pidCount = firstCat.productIds.Count();
            GetTraderXLogger().LogDebug("GetTraderCategoriesResponse:: first cat id=" + firstCat.categoryId + " name=" + firstCat.categoryName + " productIds=" + pidCount);
        }
        TraderXCategoryRepository.SetCategories(data.param1);
    }

    void GetTraderProductsResponse(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if(type != CallType.Client)
            return;

        Param3<array<ref TraderXProduct>, bool, bool> data = new Param3<array<ref TraderXProduct>, bool, bool>(null, false, false);
        if(!ctx.Read(data)){
            GetTraderXLogger().LogError("GetTraderProductsResponse:: failed to read data");
            return;
        }

        // Products stream in per-trader when a menu opens. Always MERGE (never clear) so a
        // trader's products stay cached and products from other traders opened this session are
        // preserved — product ids are unique and immutable, so re-adding is idempotent.
        // param2 (isFirst) / param3 (isLast) are kept for the wire format but need no special
        // handling here; the server sends stock after the last chunk to gate the client render.
        int chunkCount = 0;
        if (data.param1) chunkCount = data.param1.Count();
        string firstPid = "";
        if (chunkCount > 0 && data.param1[0]) firstPid = data.param1[0].productId;
        GetTraderXLogger().LogDebug("GetTraderProductsResponse:: chunk=" + chunkCount + " isFirst=" + data.param2 + " isLast=" + data.param3 + " firstPid=" + firstPid);
        TraderXProductRepository.AddProducts(data.param1);
    }

    void SendGeneralConfig(PlayerIdentity identity)
    {
        GetRPCManager().SendRPC("TraderX", "GetConfigResponse", new Param1<TraderXGeneralSettings>(generalSettings), true, identity);
    }

    void SendTraderXCategories(PlayerIdentity identity)
    {
        GetRPCManager().SendRPC("TraderX", "GetTraderCategoriesResponse", new Param1<array<ref TraderXCategory>>(TraderXCategoryRepository.GetCategories()), true, identity);
    }

    // A trader's catalog can still be large (hundreds of products). Sending it in ONE RPC
    // serialises the whole array in a single frame on both ends — a hitch. Stream it in small
    // chunks across frames instead so neither side blocks. Only the opened trader's subset is
    // sent (not the whole game catalog), and only when its menu opens — never at join.
    static const int PRODUCT_SYNC_CHUNK_SIZE = 100;
    static const int PRODUCT_SYNC_CHUNK_DELAY_MS = 50;

    // The trader's tradable products (category products + their variants + preset attachments),
    // deterministic order so the chunk slicing below is stable across CallLater re-entries.
    array<ref TraderXProduct> GetNpcProducts(int npcId)
    {
        array<ref TraderXProduct> products = new array<ref TraderXProduct>();

        TraderXNpc npc = generalSettings.GetNpcById(npcId);
        if (!npc)
            return products;

        array<string> productIds = new array<string>();
        TraderXProductStockRepository.CollectTradableProductIds(npc.categoriesId, productIds);

        foreach (string productId : productIds)
        {
            TraderXProduct product = TraderXProductRepository.GetItemById(productId);
            if (product)
                products.Insert(product);
        }
        return products;
    }

    // sendStock: true for a real menu open (stock follows the last product chunk to gate the
    // client render); false for a safezone prewarm (products only — the player isn't opening
    // this trader yet, so sending stock would fire the client's "catalog ready" gate early).
    void SendNpcProductChunk(PlayerIdentity identity, int npcId, int startIndex, bool sendStock)
    {
        if (!identity)
            return;

        array<ref TraderXProduct> products = GetNpcProducts(npcId);
        int total = products.Count();

        int end = startIndex + PRODUCT_SYNC_CHUNK_SIZE;
        if (end > total)
            end = total;

        array<ref TraderXProduct> chunk = new array<ref TraderXProduct>();
        for (int i = startIndex; i < end; i++)
            chunk.Insert(products[i]);

        bool isFirst = (startIndex == 0);
        bool isLast = (end >= total);

        GetRPCManager().SendRPC("TraderX", "GetTraderProductsResponse", new Param3<array<ref TraderXProduct>, bool, bool>(chunk, isFirst, isLast), true, identity);

        if (!isLast)
        {
            GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(SendNpcProductChunk, PRODUCT_SYNC_CHUNK_DELAY_MS, false, identity, npcId, end, sendStock);
        }
        else if (sendStock)
        {
            // All products delivered — now send stock. Sent last so it lands after every
            // product chunk on the ordered channel; the client's render gate waits on stock.
            PlayerBase player = TraderXHelper.GetPlayerByIdentity(identity);
            if (player)
                SendTraderStockToClient(player, npcId);
        }
        else
        {
            // Prewarm stream for this trader finished (no stock). Log total duration once every
            // trader in the entered zone has finished streaming.
            OnPrewarmNpcStreamComplete(identity);
        }
    }

    // Debug: one trader's prewarm stream finished — when the last of the zone's traders is done,
    // log how long the whole prewarm took.
    void OnPrewarmNpcStreamComplete(PlayerIdentity identity)
    {
        if (!identity)
            return;

        string steamId = identity.GetPlainId();
        TraderXPrewarmTiming timing = m_PrewarmTiming.Get(steamId);
        if (!timing)
            return;

        timing.pendingNpcs = timing.pendingNpcs - 1;
        if (timing.pendingNpcs > 0)
            return;

        float elapsedMs = GetGame().GetTime() - timing.startTimeMs;
        GetTraderXLogger().LogDebug(string.Format("[Prewarm] DONE — sent %1 product(s) to %2 in %3 ms", timing.totalProducts, identity.GetName(), elapsedMs));
        m_PrewarmTiming.Remove(steamId);
    }

    // Safezone prewarm (option prewarmCatalogInSafeZone): stream the products of every trader
    // inside the entered zone to the client in the background, so opening any of them is instant.
    // Products only (sendStock=false); stock is fetched fresh when the player actually opens one.
    void PrewarmCatalogInZone(PlayerIdentity identity, vector zonePos, float zoneRadius)
    {
        if (!identity)
            return;

        GetTraderXLogger().LogDebug(string.Format("[Prewarm] %1 entered safezone (radius %2)", identity.GetName(), zoneRadius));

        if (!generalSettings || !generalSettings.prewarmCatalogInSafeZone)
        {
            GetTraderXLogger().LogDebug("[Prewarm] disabled (prewarmCatalogInSafeZone=false) — skipping");
            return;
        }

        // Collect the traders inside the zone and count their products up front (debug metrics).
        vector flatZonePos = Vector(zonePos[0], 0, zonePos[2]);
        array<int> npcIdsInZone = new array<int>();
        int totalProducts = 0;
        foreach (TraderXNpc npc : generalSettings.traders)
        {
            if (!npc)
                continue;
            vector npcPos = npc.GetPosition();
            vector flatNpcPos = Vector(npcPos[0], 0, npcPos[2]);
            if (vector.Distance(flatNpcPos, flatZonePos) <= zoneRadius)
            {
                npcIdsInZone.Insert(npc.GetNpcId());
                totalProducts += GetNpcProducts(npc.GetNpcId()).Count();
            }
        }

        string steamId = identity.GetPlainId();
        TraderXPrewarmTiming timing = new TraderXPrewarmTiming();
        timing.startTimeMs = GetGame().GetTime();
        timing.pendingNpcs = npcIdsInZone.Count();
        timing.totalProducts = totalProducts;
        m_PrewarmTiming.Set(steamId, timing);

        GetTraderXLogger().LogDebug(string.Format("[Prewarm] START — streaming %1 product(s) across %2 trader(s) to %3", totalProducts, npcIdsInZone.Count(), identity.GetName()));

        if (npcIdsInZone.Count() == 0)
        {
            m_PrewarmTiming.Remove(steamId);
            return;
        }

        foreach (int npcId : npcIdsInZone)
            SendNpcProductChunk(identity, npcId, 0, false);
    }

    void OnPlayerJoined(PlayerBase player, PlayerIdentity identity)
    {
        SendGeneralConfig(identity);
        SendTraderXCategories(identity);
        // Products are NOT sent at join. Shipping the whole catalog (thousands of products)
        // here froze the client through the join window even when chunked. They're now streamed
        // per-NPC when a trader menu opens (SendNpcProductChunk), so join stays light. Category
        // cards render from the category productId lists sent above; item cards need products
        // only on expand, by which point the NPC's subset has arrived.
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