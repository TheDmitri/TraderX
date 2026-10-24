// TraderX Configuration Service
// Handles all configuration loading, format selection, and migration logic
// Separated from TraderXModule to follow Single Responsibility Principle

class TraderXConfigurationService
{
    private static ref TraderXConfigurationService s_Instance;
    
    static TraderXConfigurationService GetInstance()
    {
        if (!s_Instance)
        {
            s_Instance = new TraderXConfigurationService();
        }
        return s_Instance;
    }
    
    // Main entry point: Initialize configuration based on user preferences.
    // importedThisBoot: an importer (TraderPlus / Dr Jones / Expansion) just wrote per-file JSON products.
    void Initialize(TraderXSourceConfig sourceConfig, bool importedThisBoot = false)
    {
        ETraderXSourceFormat format = sourceConfig.GetFormatEnum();

        GetTraderXLogger().LogInfo(string.Format("[TraderX] Configuration format selected: %1", sourceConfig.preferredFormat));

        switch (format)
        {
            case ETraderXSourceFormat.LEGACY:
                LoadLegacyCatalog();
                break;

            case ETraderXSourceFormat.CSV:
                LoadCsvConfiguration(sourceConfig, importedThisBoot);
                break;

            case ETraderXSourceFormat.COMPILED:
                LoadCompiledConfiguration(sourceConfig, importedThisBoot);
                break;
        }

        TraderXVehicleParkingRepository.LoadAllParkingCollections();
        ApplyStockBehaviorAtRestart();
        GetTraderXLogger().LogInfo(string.Format("[TraderX] Catalog loaded: %1 products, %2 categories", TraderXProductRepository.GetProducts().Count(), TraderXCategoryRepository.GetCategories().Count()));
    }

    // LEGACY format: Products/*.json + Categories/*.json (one file per entry, the file name is the id)
    private void LoadLegacyCatalog()
    {
        GetTraderXLogger().LogInfo("[TraderX] Loading the per-file JSON catalog (Products/, Categories/)");

        TraderXProductRepository.LoadAllProducts(false);
        TraderXCategoryRepository.LoadAllCategories(false);

        // DayZ 1.30 fallback: when -profiles is an external/absolute path where FindFile returns 0,
        // walk the trader -> categoryId -> productId/variant/attachment graph via exact $profile: paths.
        if (TraderXCategoryRepository.GetCategories().Count() == 0 || TraderXProductRepository.GetProducts().Count() == 0)
        {
            LoadLegacyGraphFromSettings();
        }
    }

    private void LoadLegacyGraphFromSettings()
    {
        TraderXGeneralSettings settings = TraderXSettingsRepository.Load();
        if (!settings || !settings.traders)
            return;

        foreach (TraderXNpc npc : settings.traders)
        {
            if (!npc || !npc.categoriesId)
                continue;
            foreach (string catId : npc.categoriesId)
            {
                if (catId == "")
                    continue;
                TraderXCategory cat = TraderXCategoryRepository.LoadCategoryById(catId);
                if (!cat || !cat.productIds)
                    continue;
                foreach (string prodId : cat.productIds)
                {
                    if (prodId != "")
                        TraderXProductRepository.LoadProductByIdRecursive(prodId);
                }
            }
        }
        GetTraderXLogger().LogInfo(string.Format("[TraderX] Direct graph fallback loaded %1 categories and %2 products without FindFile", TraderXCategoryRepository.GetCategories().Count(), TraderXProductRepository.GetProducts().Count()));
    }

    // CSV format: Source/*.csv is the catalog, compiled to Compiled/ at each boot and loaded from there
    private void LoadCsvConfiguration(TraderXSourceConfig sourceConfig, bool importedThisBoot)
    {
        GetTraderXLogger().LogInfo("[TraderX] Loading configuration using CSV format");

        if (!TraderXConfigCompiler.HasSourceFiles())
        {
            if (sourceConfig.autoMigrate)
            {
                // First CSV boot: turn the per-file JSON catalog (own or just imported) into Source/*.csv
                MigrateLegacyToCsv("products.csv", "categories.csv", "presets.csv");
                sourceConfig.autoMigrate = false;
                TraderXSourceConfigLoader.SaveConfig(sourceConfig);
            }
            if (!TraderXConfigCompiler.HasSourceFiles())
            {
                TraderXJsonToCsvConverter.WriteEmptyTemplates();
                GetTraderXLogger().LogWarning("[TraderX] Source/ is empty: wrote header-only products.csv, categories.csv and presets.csv to fill in");
            }
        }
        else if (importedThisBoot)
        {
            // Importers write per-file JSON. Hand it to the compiler as the lowest-priority pack: rows the admin
            // already has in Source/ win, new imported products and categories are added.
            MigrateLegacyToCsv("00_imported_products.csv", "00_imported_categories.csv", "00_imported_presets.csv");
            GetTraderXLogger().LogWarning("[TraderX] Imported catalog added to Source/00_imported_*.csv: review it, then move what you keep into your own CSV files");
        }

        if (sourceConfig.allowCompilation)
        {
            if (!TraderXConfigCompiler.CompileAndValidate())
                GetTraderXLogger().LogWarning("[TraderX] CSV not compiled this boot (errors or empty Source/). See TraderXLogs/Config/ConfigReport.log; the last good Compiled/ catalog is used.");
        }

        TraderXProductRepository.LoadAllProducts(true);
        TraderXCategoryRepository.LoadAllCategories(true);
    }

    // COMPILED format: Compiled/*_compiled.json is edited directly (created once from Source/ or the per-file JSON)
    private void LoadCompiledConfiguration(TraderXSourceConfig sourceConfig, bool importedThisBoot)
    {
        GetTraderXLogger().LogInfo("[TraderX] Loading configuration using COMPILED format");

        bool hasCompiledFiles = FileExist(TRADERX_COMPILED_PRODUCTS_FILE) && FileExist(TRADERX_COMPILED_CATEGORIES_FILE);
        if (!hasCompiledFiles && sourceConfig.autoMigrate)
        {
            if (!TraderXConfigCompiler.HasSourceFiles())
                MigrateLegacyToCsv("products.csv", "categories.csv", "presets.csv");
            if (TraderXConfigCompiler.CompileAndValidate())
            {
                sourceConfig.autoMigrate = false;
                TraderXSourceConfigLoader.SaveConfig(sourceConfig);
            }
        }
        else if (importedThisBoot)
        {
            GetTraderXLogger().LogWarning("[TraderX] COMPILED format: the catalog just imported into Products/ and Categories/ is not used. Switch to CSV to merge it.");
        }

        TraderXProductRepository.LoadAllProducts(true);
        TraderXCategoryRepository.LoadAllCategories(true);
    }

    private void MigrateLegacyToCsv(string productsFile, string categoriesFile, string presetsFile)
    {
        LoadLegacyCatalog();
        int exported = TraderXJsonToCsvConverter.ExportLoadedCatalog(TRADERX_CSV_SOURCE_DIR + productsFile, TRADERX_CSV_SOURCE_DIR + categoriesFile, TRADERX_CSV_SOURCE_DIR + presetsFile);
        if (exported == 0)
            return;

        string note = "This server uses the CSV catalog: edit TraderXConfig/Source/*.csv. The files in this folder are kept as a backup and are no longer read.";
        WriteNote(TRADERX_PRODUCTS_DIR + "_NOT_USED_SEE_Source.txt", note);
        WriteNote(TRADERX_CATEGORIES_DIR + "_NOT_USED_SEE_Source.txt", note);
    }

    private void WriteNote(string path, string text)
    {
        FileHandle file = OpenFile(path, FileMode.WRITE);
        if (!file)
            return;
        FPrintln(file, text);
        CloseFile(file);
    }

    // Apply stock behavior at restart for all products (reset, random, destock)
    private void ApplyStockBehaviorAtRestart()
    {
        array<ref TraderXProduct> products = TraderXProductRepository.GetProducts();
        int processed = 0;
        
        foreach(TraderXProduct product: products)
        {
            if (!product || product.IsStockUnlimited())
                continue;
            
            product.LoadCategoryStock();
            processed++;
        }

        GetTraderXLogger().LogInfo(string.Format("[TraderX] ApplyStockBehaviorAtRestart - Processed %1 products", processed));

        WarmStockCacheAtBoot();
    }

    // Preload every finite-stock file into the in-memory cache at server boot.
    // Products with stockBehavior 0 (persist stock across restart, the common case)
    // are NOT touched by the loop above, so their stock files were previously read
    // lazily on the FIRST trader-menu open: ~500 synchronous JsonFileLoader reads on
    // the main thread froze the whole server ~5s (queued pickups/shots/movement then
    // fired at once). Doing the read here — during the deferred boot init, with no
    // players yet acting — moves that one-time cost off the gameplay path.
    //
    // Warm straight from the product repository (all products are already loaded above),
    // NOT via categories: at boot the category->product links aren't built yet, so walking
    // categories would dereference an unpopulated product list. GetProducts() is also a
    // superset (includes variants/attachments), so this warms more than the menu path did.
    private void WarmStockCacheAtBoot()
    {
        array<ref TraderXProduct> products = TraderXProductRepository.GetProducts();
        if (!products)
            return;

        array<string> productIdsToWarm = new array<string>();
        foreach(TraderXProduct product: products)
        {
            if (product && !product.IsStockUnlimited())
                productIdsToWarm.Insert(product.GetProductId());
        }

        TraderXProductStockRepository.LoadStockBulk(productIdsToWarm);
        GetTraderXLogger().LogInfo(string.Format("[TraderX] WarmStockCacheAtBoot - Warmed stock for %1 products", productIdsToWarm.Count()));
    }
}
