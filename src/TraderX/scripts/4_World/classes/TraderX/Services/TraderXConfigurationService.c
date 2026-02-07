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
    
    // Main entry point: Initialize configuration based on user preferences
    void Initialize(TraderXSourceConfig sourceConfig)
    {
        ETraderXSourceFormat format = sourceConfig.GetFormatEnum();
        
        GetTraderXLogger().LogInfo(string.Format("[TraderX] Configuration format selected: %1", sourceConfig.preferredFormat));
        
        // Handle configuration based on user's choice
        switch (format)
        {
            case ETraderXSourceFormat.LEGACY:
                LoadLegacyConfiguration();
                break;
                
            case ETraderXSourceFormat.CSV:
                LoadCsvConfiguration(sourceConfig);
                break;
                
            case ETraderXSourceFormat.COMPILED:
                LoadCompiledConfiguration(sourceConfig);
                break;
        }
    }
    
    // Load configuration using LEGACY format (individual JSON files)
    private void LoadLegacyConfiguration()
    {
        GetTraderXLogger().LogInfo("[TraderX] Loading configuration using LEGACY format (individual JSON files)");
        
        TraderXProductRepository.LoadAllProducts();
        TraderXCategoryRepository.LoadAllCategories();
        TraderXVehicleParkingRepository.LoadAllParkingCollections();
        ApplyStockBehaviorAtRestart();
        
        GetTraderXLogger().LogInfo("[TraderX] Legacy configuration loaded successfully");
    }
    
    // Load configuration using CSV format (Source/*.csv files)
    private void LoadCsvConfiguration(TraderXSourceConfig sourceConfig)
    {
        GetTraderXLogger().LogInfo("[TraderX] Loading configuration using CSV format");
        
        // Check if source files exist, if not and auto-migrate is enabled, migrate from legacy
        if (!HasSourceFiles())
        {
            if (sourceConfig.autoMigrate && HasLegacyConfigFiles())
            {
                GetTraderXLogger().LogInfo("[TraderX] No CSV source files found, auto-migrating from legacy...");
                TraderXJsonToCsvConverter.ConvertAllJsonToCsv();
                
                // Disable autoMigrate after successful migration
                if (HasSourceFiles())
                {
                    sourceConfig.autoMigrate = false;
                    TraderXSourceConfigLoader.SaveConfig(sourceConfig);
                    GetTraderXLogger().LogInfo("[TraderX] Migration complete. autoMigrate automatically disabled to prevent re-conversion.");
                }
            }
            else
            {
                GetTraderXLogger().LogWarning("[TraderX] No CSV source files found and auto-migrate disabled or no legacy files");
            }
        }
        
        // Compile and validate CSV configuration
        if (sourceConfig.allowCompilation)
        {
            bool compilationSuccess = TraderXConfigCompiler.CompileAndValidate();
            if (!compilationSuccess)
            {
                GetTraderXLogger().LogWarning("[TraderX] CSV compilation failed or used backup. Check ConfigReport.log for details.");
            }
        }
        
        // Load from compiled JSON (which was just created from CSV)
        TraderXProductRepository.LoadAllProducts();
        TraderXCategoryRepository.LoadAllCategories();
        TraderXVehicleParkingRepository.LoadAllParkingCollections();
        ApplyStockBehaviorAtRestart();
        
        GetTraderXLogger().LogInfo("[TraderX] CSV configuration loaded successfully");
    }
    
    // Load configuration using COMPILED format (Compiled/*.json files)
    private void LoadCompiledConfiguration(TraderXSourceConfig sourceConfig)
    {
        GetTraderXLogger().LogInfo("[TraderX] Loading configuration using COMPILED format");
        
        // Check if compiled files exist
        bool hasCompiledFiles = FileExist(TRADERX_COMPILED_PRODUCTS_FILE) && FileExist(TRADERX_COMPILED_CATEGORIES_FILE);
        
        // If no compiled files but auto-migrate is enabled, try to create them from CSV or legacy
        if (!hasCompiledFiles && sourceConfig.autoMigrate)
        {
            if (HasSourceFiles())
            {
                GetTraderXLogger().LogInfo("[TraderX] No compiled files found, compiling from source...");
                TraderXConfigCompiler.CompileAndValidate();
            }
            else if (HasLegacyConfigFiles())
            {
                GetTraderXLogger().LogInfo("[TraderX] No compiled or source files found, migrating from legacy to CSV then compiling...");
                TraderXJsonToCsvConverter.ConvertAllJsonToCsv();
                TraderXConfigCompiler.CompileAndValidate();
            }
            
            // Disable autoMigrate after successful migration/compilation
            bool nowHasCompiledFiles = FileExist(TRADERX_COMPILED_PRODUCTS_FILE) && FileExist(TRADERX_COMPILED_CATEGORIES_FILE);
            if (nowHasCompiledFiles)
            {
                sourceConfig.autoMigrate = false;
                TraderXSourceConfigLoader.SaveConfig(sourceConfig);
                GetTraderXLogger().LogInfo("[TraderX] Migration/Compilation complete. autoMigrate automatically disabled to prevent re-conversion.");
            }
        }
        
        // Load from compiled JSON
        TraderXProductRepository.LoadAllProducts();
        TraderXCategoryRepository.LoadAllCategories();
        TraderXVehicleParkingRepository.LoadAllParkingCollections();
        ApplyStockBehaviorAtRestart();
        
        GetTraderXLogger().LogInfo("[TraderX] Compiled configuration loaded successfully");
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
    }
    
    // Helper: Check if Source directory has any CSV or JSON files
    private bool HasSourceFiles()
    {
        string filename;
        FileAttr attr;

        // Check for CSV files
        FindFileHandle csvHandle = FindFile(TRADERX_CSV_SOURCE_DIR + "*.csv", filename, attr, FindFileFlags.ALL);
        if (csvHandle != 0)
        {
            CloseFindFile(csvHandle);
            return true;
        }

        // Check for JSON files
        FindFileHandle jsonHandle = FindFile(TRADERX_CSV_SOURCE_DIR + "*.json", filename, attr, FindFileFlags.ALL);
        if (jsonHandle != 0)
        {
            CloseFindFile(jsonHandle);
            return true;
        }

        return false;
    }
    
    // Helper: Check if legacy config files exist (individual JSON files)
    private bool HasLegacyConfigFiles()
    {
        // Check for legacy product files
        if (FileExist(TRADERX_PRODUCTS_DIR))
        {
            string filename1;
            FileAttr attr;
            FindFileHandle findHandle1 = FindFile(TRADERX_PRODUCTS_DIR + "*.json", filename1, attr, FindFileFlags.ALL);
            if (findHandle1 != 0)
            {
                CloseFindFile(findHandle1);
                return true;
            }
        }

        // Check for legacy category files
        if (FileExist(TRADERX_CATEGORIES_DIR))
        {
            string filename2;
            FileAttr attr2;
            FindFileHandle findHandle2 = FindFile(TRADERX_CATEGORIES_DIR + "*.json", filename2, attr2, FindFileFlags.ALL);
            if (findHandle2 != 0)
            {
                CloseFindFile(findHandle2);
                return true;
            }
        }

        return false;
    }
}
