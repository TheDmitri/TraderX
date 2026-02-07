/**
 * TraderX Configuration Compiler
 * 
 * Responsibility: Orchestrate the compilation of TraderX configuration from source to runtime format.
 * 
 * This class coordinates the following process:
 * 1. Load user preferences (CSV vs JSON)
 * 2. Detect current source format
 * 3. Auto-migrate if needed
 * 4. Load and merge source files
 * 5. Validate configuration
 * 6. Compile to optimized runtime JSON
 * 7. Create backups
 * 
 * Domain Services Used:
 * - TraderXSourceFormatDetector: Detects CSV/JSON format
 * - TraderXFormatMigrator: Handles format migrations
 * - TraderXBitfieldPacker: Packs/unpacks bitfield data
 * - TraderXConfigValidator: Validates configuration rules
 * 
 * @author TraderX Team
 * @since 2.0 (CSV System)
 */
class TraderXConfigCompiler
{
    // Main compilation entry point
    static bool CompileAndValidate()
    {
        GetTraderXLogger().LogInfo("=== TraderX Configuration Compilation Started ===");
        
        // Load user preferences
        TraderXSourceConfig sourceConfig = TraderXSourceConfigLoader.LoadConfig();
        
        // Ensure directories exist
        if (!FileExist(TRADERX_CSV_SOURCE_DIR))
        {
            GetTraderXLogger().LogWarning("Source directory not found, creating: " + TRADERX_CSV_SOURCE_DIR);
            MakeDirectory(TRADERX_CSV_SOURCE_DIR);
            return false;
        }
        
        if (!FileExist(TRADERX_COMPILED_DIR))
        {
            MakeDirectory(TRADERX_COMPILED_DIR);
        }
        
        if (!FileExist(TRADERX_BACKUP_DIR))
        {
            MakeDirectory(TRADERX_BACKUP_DIR);
        }
        
        // Use domain services for format detection and migration
        TraderXSourceFormatDetector formatDetector = new TraderXSourceFormatDetector(TRADERX_CSV_SOURCE_DIR);
        string currentFormat = formatDetector.DetectFormat();
        
        // Check if migration is needed
        if (sourceConfig.autoMigrate && formatDetector.NeedsMigration(sourceConfig.preferredFormat, currentFormat))
        {
            TraderXFormatMigrator migrator = new TraderXFormatMigrator(TRADERX_CSV_SOURCE_DIR);
            if (!migrator.MigrateToFormat(sourceConfig.preferredFormat))
            {
                GetTraderXLogger().LogError("Format migration failed");
                return false;
            }
        }
        
        // Warn if mixed formats detected
        if (formatDetector.HasMixedFormats())
        {
            GetTraderXLogger().LogWarning("Both CSV and JSON files found in Source - CSV takes priority");
        }
        
        // Detect source format and load
        array<ref TraderXCsvProduct> products = new array<ref TraderXCsvProduct>;
        array<ref TraderXCsvCategory> categories = new array<ref TraderXCsvCategory>;
        array<ref TraderXCsvPreset> presets = new array<ref TraderXCsvPreset>;
        
        bool loadSuccess = LoadSourceFiles(products, categories, presets);
        
        if (!loadSuccess)
        {
            GetTraderXLogger().LogError("Failed to load source files");
            return RestoreFromBackup();
        }
        
        // Validate configuration
        TraderXConfigValidator validator = new TraderXConfigValidator();
        validator.ValidateProducts(products);
        validator.ValidateCategories(categories, products);
        validator.ValidatePresets(presets, products);
        validator.ValidateCrossReferences(products);
        
        // Generate validation report
        string reportPath = TRADERX_CONFIG_REPORT_FILE;
        validator.GenerateReport(reportPath);
        
        if (!validator.IsValid())
        {
            GetTraderXLogger().LogError(string.Format("Configuration validation failed with %1 errors. See report: %2", validator.GetErrorCount(), reportPath));
            return RestoreFromBackup();
        }
        
        GetTraderXLogger().LogInfo(string.Format("Validation passed. Errors: 0, Warnings: %1", validator.GetWarningCount()));
        
        // Compile to JSON
        bool compileSuccess = CompileToJson(products, categories, presets);
        
        if (!compileSuccess)
        {
            GetTraderXLogger().LogError("Compilation failed");
            return RestoreFromBackup();
        }
        
        // Create backup of successful compilation
        CreateBackup();
        
        GetTraderXLogger().LogInfo("=== TraderX Configuration Compilation Complete ===");
        return true;
    }
    
    // Auto-detect source format and load files
    private static bool LoadSourceFiles(out array<ref TraderXCsvProduct> products, out array<ref TraderXCsvCategory> categories, out array<ref TraderXCsvPreset> presets)
    {
        GetTraderXLogger().LogInfo("Detecting source format in: " + TRADERX_CSV_SOURCE_DIR);
        
        // Check for CSV files first
        string filename;
        FileAttr attr;
        FindFileHandle csvHandle = FindFile(TRADERX_CSV_SOURCE_DIR + "*.csv", filename, attr, FindFileFlags.ALL);
        
        if (csvHandle != 0)
        {
            CloseFindFile(csvHandle);
            GetTraderXLogger().LogInfo("CSV source files detected - using CSV mode");
            return LoadCsvPacks(products, categories, presets);
        }
        
        // Check for JSON files
        FindFileHandle jsonHandle = FindFile(TRADERX_CSV_SOURCE_DIR + "*.json", filename, attr, FindFileFlags.ALL);
        
        if (jsonHandle != 0)
        {
            CloseFindFile(jsonHandle);
            GetTraderXLogger().LogInfo("JSON source files detected - using JSON mode");
            return LoadJsonSource(products, categories, presets);
        }
        
        GetTraderXLogger().LogError("No CSV or JSON source files found in Source directory");
        return false;
    }
    
    // Load and merge CSV packs (alphabetical order)
    private static bool LoadCsvPacks(out array<ref TraderXCsvProduct> products, out array<ref TraderXCsvCategory> categories, out array<ref TraderXCsvPreset> presets)
    {
        GetTraderXLogger().LogInfo("Loading CSV packs from: " + TRADERX_CSV_SOURCE_DIR);
        
        // Find all CSV files
        array<ref TraderXCsvPackInfo> packs = new array<ref TraderXCsvPackInfo>;
        string filename;
        FileAttr attr;
        
        FindFileHandle findHandle = FindFile(TRADERX_CSV_SOURCE_DIR + "*.csv", filename, attr, FindFileFlags.ALL);
        
        if (findHandle == 0)
        {
            GetTraderXLogger().LogError("No CSV files found in source directory");
            CloseFindFile(findHandle);
            return false;
        }
        
        // Collect all CSV files
        packs.Insert(new TraderXCsvPackInfo(filename));
        
        while (FindNextFile(findHandle, filename, attr))
        {
            packs.Insert(new TraderXCsvPackInfo(filename));
        }
        
        CloseFindFile(findHandle);
        
        // Sort packs by priority (alphabetical = priority)
        SortPacksByPriority(packs);
        
        GetTraderXLogger().LogInfo(string.Format("Found %1 CSV pack(s)", packs.Count()));
        
        // Load and merge packs
        map<string, ref TraderXCsvProduct> productMap = new map<string, ref TraderXCsvProduct>;
        map<string, ref TraderXCsvCategory> categoryMap = new map<string, ref TraderXCsvCategory>;
        map<string, ref TraderXCsvPreset> presetMap = new map<string, ref TraderXCsvPreset>;
        
        foreach (TraderXCsvPackInfo pack : packs)
        {
            string filePath = TRADERX_CSV_SOURCE_DIR + pack.fileName;
            GetTraderXLogger().LogInfo(string.Format("Loading pack: %1 (priority: %2)", pack.fileName, pack.priority));
            
            // Determine file type by name
            string lowerName = pack.fileName;
            lowerName.ToLower();
            
            if (lowerName.Contains("product"))
            {
                array<ref TraderXCsvProduct> packProducts = TraderXCsvLoader.LoadProductsCsv(filePath);
                MergeProducts(productMap, packProducts);
            }
            else if (lowerName.Contains("categor"))
            {
                array<ref TraderXCsvCategory> packCategories = TraderXCsvLoader.LoadCategoriesCsv(filePath);
                MergeCategories(categoryMap, packCategories);
            }
            else if (lowerName.Contains("preset"))
            {
                array<ref TraderXCsvPreset> packPresets = TraderXCsvLoader.LoadPresetsCsv(filePath);
                MergePresets(presetMap, packPresets);
            }
            else
            {
                GetTraderXLogger().LogWarning("Unknown CSV file type (name should contain 'product', 'categor', or 'preset'): " + pack.fileName);
            }
        }
        
        // Convert maps to arrays
        foreach (string prodKey, TraderXCsvProduct product : productMap)
        {
            products.Insert(product);
        }
        
        foreach (string catKey, TraderXCsvCategory category : categoryMap)
        {
            categories.Insert(category);
        }
        
        foreach (string presetKey, TraderXCsvPreset preset : presetMap)
        {
            presets.Insert(preset);
        }
        
        GetTraderXLogger().LogInfo(string.Format("Loaded totals - Products: %1, Categories: %2, Presets: %3", products.Count(), categories.Count(), presets.Count()));
        
        return products.Count() > 0;
    }
    
    // Load JSON source files from Source directory
    private static bool LoadJsonSource(out array<ref TraderXCsvProduct> products, out array<ref TraderXCsvCategory> categories, out array<ref TraderXCsvPreset> presets)
    {
        GetTraderXLogger().LogInfo("Loading JSON source files from: " + TRADERX_CSV_SOURCE_DIR);
        
        // Load products JSON
        string productsJsonPath = TRADERX_CSV_SOURCE_DIR + "products.json";
        if (FileExist(productsJsonPath))
        {
            ref array<ref TraderXCompiledProduct> jsonProducts = new array<ref TraderXCompiledProduct>;
            JsonFileLoader<array<ref TraderXCompiledProduct>>.JsonLoadFile(productsJsonPath, jsonProducts);
            
            if (jsonProducts && jsonProducts.Count() > 0)
            {
                GetTraderXLogger().LogInfo(string.Format("Loaded %1 products from JSON", jsonProducts.Count()));
                
                foreach (TraderXCompiledProduct jsonProd : jsonProducts)
                {
                    TraderXCsvProduct csvProd = ConvertJsonProductToCsv(jsonProd);
                    if (csvProd)
                    {
                        products.Insert(csvProd);
                    }
                }
            }
        }
        
        // Load categories JSON
        string categoriesJsonPath = TRADERX_CSV_SOURCE_DIR + "categories.json";
        if (FileExist(categoriesJsonPath))
        {
            ref array<ref TraderXCompiledCategory> jsonCategories = new array<ref TraderXCompiledCategory>;
            JsonFileLoader<array<ref TraderXCompiledCategory>>.JsonLoadFile(categoriesJsonPath, jsonCategories);
            
            if (jsonCategories && jsonCategories.Count() > 0)
            {
                GetTraderXLogger().LogInfo(string.Format("Loaded %1 categories from JSON", jsonCategories.Count()));
                
                foreach (TraderXCompiledCategory jsonCat : jsonCategories)
                {
                    TraderXCsvCategory csvCat = ConvertJsonCategoryToCsv(jsonCat);
                    if (csvCat)
                    {
                        categories.Insert(csvCat);
                    }
                }
            }
        }
        
        if (products.Count() == 0 && categories.Count() == 0)
        {
            GetTraderXLogger().LogError("No valid JSON data loaded from Source directory");
            return false;
        }
        
        GetTraderXLogger().LogInfo(string.Format("Loaded totals - Products: %1, Categories: %2", products.Count(), categories.Count()));
        return true;
    }
    
    // Convert JSON product to CSV model (unpack bitfields using domain service)
    private static TraderXCsvProduct ConvertJsonProductToCsv(TraderXCompiledProduct jsonProd)
    {
        TraderXCsvProduct csvProd = new TraderXCsvProduct();
        
        csvProd.productKey = jsonProd.productId;
        csvProd.className = jsonProd.className;
        csvProd.buyPrice = jsonProd.buyPrice;
        csvProd.sellPrice = jsonProd.sellPrice;
        csvProd.maxStock = jsonProd.maxStock;
        
        // Unpack tradeQuantity bitfield using domain service
        string buyMode;
        float buyValue;
        string sellMode;
        float sellValue;
        
        TraderXBitfieldPacker.UnpackTradeQuantity(jsonProd.tradeQuantity, buyMode, buyValue, sellMode, sellValue);
        
        csvProd.buyQtyMode = buyMode;
        csvProd.buyQtyValue = buyValue;
        csvProd.sellQtyMode = sellMode;
        csvProd.sellQtyValue = sellValue;
        
        // Unpack stockSettings bitfield using domain service
        float destockCoef;
        int stockBehavior;
        
        TraderXBitfieldPacker.UnpackStockSettings(jsonProd.stockSettings, destockCoef, stockBehavior);
        
        csvProd.destockCoefficient = destockCoef;
        csvProd.stockBehaviorAtRestart = stockBehavior;
        
        // Copy arrays
        csvProd.attachments = jsonProd.attachments;
        csvProd.variants = jsonProd.variants;
        
        return csvProd;
    }
    
    // Convert JSON category to CSV model
    private static TraderXCsvCategory ConvertJsonCategoryToCsv(TraderXCompiledCategory jsonCat)
    {
        TraderXCsvCategory csvCat = new TraderXCsvCategory();
        
        csvCat.categoryKey = jsonCat.categoryId;
        csvCat.categoryName = jsonCat.categoryName;
        csvCat.icon = jsonCat.icon;
        csvCat.isVisible = jsonCat.isVisible;
        csvCat.licensesRequired = jsonCat.licensesRequired;
        csvCat.productKeys = jsonCat.productIds;
        
        return csvCat;
    }
    
    // Compile CSV data to JSON format
    private static bool CompileToJson(array<ref TraderXCsvProduct> products, array<ref TraderXCsvCategory> categories, array<ref TraderXCsvPreset> presets)
    {
        GetTraderXLogger().LogInfo("Compiling CSV to JSON...");
        
        // Compile products
        ref array<ref TraderXCompiledProduct> jsonProducts = new array<ref TraderXCompiledProduct>;
        
        foreach (TraderXCsvProduct csvProduct : products)
        {
            TraderXCompiledProduct jsonProduct = new TraderXCompiledProduct();
            
            jsonProduct.productId = csvProduct.productKey;
            jsonProduct.className = csvProduct.className;
            jsonProduct.buyPrice = csvProduct.buyPrice;
            jsonProduct.sellPrice = csvProduct.sellPrice;
            jsonProduct.maxStock = csvProduct.maxStock;
            jsonProduct.coefficient = csvProduct.destockCoefficient;
            
            // Pack tradeQuantity bitfield using domain service
            jsonProduct.tradeQuantity = TraderXBitfieldPacker.PackTradeQuantity(csvProduct.buyQtyMode, csvProduct.buyQtyValue, csvProduct.sellQtyMode, csvProduct.sellQtyValue);
            
            // Pack stockSettings bitfield using domain service
            jsonProduct.stockSettings = TraderXBitfieldPacker.PackStockSettings(csvProduct.destockCoefficient, csvProduct.stockBehaviorAtRestart);
            
            // Copy arrays
            if (csvProduct.attachments)
            {
                jsonProduct.attachments = new array<string>;
                foreach (string att : csvProduct.attachments)
                {
                    jsonProduct.attachments.Insert(att);
                }
            }
            
            if (csvProduct.variants)
            {
                jsonProduct.variants = new array<string>;
                foreach (string var : csvProduct.variants)
                {
                    jsonProduct.variants.Insert(var);
                }
            }
            
            jsonProducts.Insert(jsonProduct);
        }
        
        // Compile categories
        ref array<ref TraderXCompiledCategory> jsonCategories = new array<ref TraderXCompiledCategory>;
        
        foreach (TraderXCsvCategory csvCategory : categories)
        {
            TraderXCompiledCategory jsonCategory = new TraderXCompiledCategory();
            
            jsonCategory.categoryId = csvCategory.categoryKey;
            jsonCategory.categoryName = csvCategory.categoryName;
            jsonCategory.icon = csvCategory.icon;
            jsonCategory.isVisible = csvCategory.isVisible;
            
            // Copy arrays
            if (csvCategory.licensesRequired)
            {
                jsonCategory.licensesRequired = new array<string>;
                foreach (string lic : csvCategory.licensesRequired)
                {
                    jsonCategory.licensesRequired.Insert(lic);
                }
            }
            
            if (csvCategory.productKeys)
            {
                jsonCategory.productIds = new array<string>;
                foreach (string prodKey : csvCategory.productKeys)
                {
                    jsonCategory.productIds.Insert(prodKey);
                }
            }
            
            jsonCategories.Insert(jsonCategory);
        }
        
        // Save compiled JSON files
        string productsPath = TRADERX_COMPILED_PRODUCTS_FILE;
        string categoriesPath = TRADERX_COMPILED_CATEGORIES_FILE;
        
        GetTraderXLogger().LogInfo("Saving compiled products to: " + productsPath);
        JsonFileLoader<array<ref TraderXCompiledProduct>>.JsonSaveFile(productsPath, jsonProducts);
        
        GetTraderXLogger().LogInfo("Saving compiled categories to: " + categoriesPath);
        JsonFileLoader<array<ref TraderXCompiledCategory>>.JsonSaveFile(categoriesPath, jsonCategories);
        
        // Compile presets if any
        if (presets.Count() > 0)
        {
            GetTraderXLogger().LogInfo(string.Format("Compiling %1 presets...", presets.Count()));
            CompilePresets(presets);
        }
        
        GetTraderXLogger().LogInfo("Compilation complete");
        return true;
    }
    
    // Pack tradeQuantity bitfield from CSV fields
    private static int PackTradeQuantity(string buyMode, float buyValue, string sellMode, float sellValue)
    {
        int packed = 0;
        
        buyMode.ToUpper();
        sellMode.ToUpper();
        
        // Pack sell mode (bits 0-2)
        if (sellMode == "NO_MATTER")
            packed = packed | 0;
        else if (sellMode == "EMPTY")
            packed = packed | 1;
        else if (sellMode == "FULL")
            packed = packed | 2;
        else if (sellMode == "COEFFICIENT")
            packed = packed | 3;
        else if (sellMode == "STATIC")
            packed = packed | 4;
        
        // Pack buy mode (bits 3-5)
        if (buyMode == "EMPTY")
            packed = packed | 8;
        else if (buyMode == "FULL")
            packed = packed | 16;
        else if (buyMode == "COEFFICIENT")
            packed = packed | 24;
        else if (buyMode == "STATIC")
            packed = packed | 32;
        
        // Pack values if needed (bits 6-31)
        if (sellMode == "COEFFICIENT" || sellMode == "STATIC")
        {
            int sellValueInt = sellValue * 1000;
            packed |= (sellValueInt << 6);
        }
        
        if (buyMode == "COEFFICIENT" || buyMode == "STATIC")
        {
            int buyValueInt = buyValue * 1000;
            packed |= (buyValueInt << 16);
        }
        
        return packed;
    }
    
    // Pack stockSettings bitfield from CSV fields
    private static int PackStockSettings(float destockCoef, int behavior)
    {
        int packed = 0;
        
        // Pack destock coefficient (bits 0-15, stored as int * 10000)
        int coefInt = destockCoef * 10000;
        packed |= (coefInt & 0xFFFF);
        
        // Pack stock behavior (bits 16-17)
        packed |= ((behavior & 0x3) << 16);
        
        return packed;
    }
    
    // Compile presets to individual JSON files
    private static void CompilePresets(array<ref TraderXCsvPreset> presets)
    {
        // Group presets by productKey
        ref map<string, ref array<ref TraderXCsvPreset>> presetsByProduct = new map<string, ref array<ref TraderXCsvPreset>>;
        
        foreach (TraderXCsvPreset csvPresetItem : presets)
        {
            ref array<ref TraderXCsvPreset> prodPresets;
            
            if (!presetsByProduct.Find(csvPresetItem.productKey, prodPresets))
            {
                prodPresets = new array<ref TraderXCsvPreset>;
                presetsByProduct.Set(csvPresetItem.productKey, prodPresets);
            }
            
            prodPresets.Insert(csvPresetItem);
        }
        
        // Save presets for each product
        foreach (string prodKey, ref array<ref TraderXCsvPreset> productPresets : presetsByProduct)
        {
            TraderXPresets presetsObj = new TraderXPresets();
            presetsObj.productId = prodKey;
            presetsObj.presets = new array<ref TraderXPreset>;
            
            foreach (TraderXCsvPreset csvPreset : productPresets)
            {
                TraderXPreset preset = new TraderXPreset();
                preset.presetName = csvPreset.presetName;
                
                if (csvPreset.attachments)
                {
                    preset.attachments = new array<string>;
                    foreach (string att : csvPreset.attachments)
                    {
                        preset.attachments.Insert(att);
                    }
                }
                
                presetsObj.presets.Insert(preset);
            }
            
            string presetPath = TRADERX_PRESETS + "presets_" + prodKey + ".json";
            string errorMessage;
            
            if (!JsonFileLoader<TraderXPresets>.SaveFile(presetPath, presetsObj, errorMessage))
            {
                GetTraderXLogger().LogError("Failed to save preset: " + presetPath + " - " + errorMessage);
            }
        }
    }
    
    // Create backup of compiled files
    private static void CreateBackup()
    {
        GetTraderXLogger().LogInfo("Creating backup of compiled configuration...");
        
        string productsSource = TRADERX_COMPILED_PRODUCTS_FILE;
        string productsBackup = TRADERX_COMPILED_PRODUCTS_BACKUP;
        
        string categoriesSource = TRADERX_COMPILED_CATEGORIES_FILE;
        string categoriesBackup = TRADERX_COMPILED_CATEGORIES_BACKUP;
        
        if (FileExist(productsSource))
        {
            CopyFile(productsSource, productsBackup);
        }
        
        if (FileExist(categoriesSource))
        {
            CopyFile(categoriesSource, categoriesBackup);
        }
        
        // Write timestamp
        FileHandle timestampFile = OpenFile(TRADERX_BACKUP_DIR + ".backup_timestamp", FileMode.WRITE);
        if (timestampFile)
        {
            FPrintln(timestampFile, "Backup created at server startup");
            CloseFile(timestampFile);
        }
        
        GetTraderXLogger().LogInfo("Backup created successfully");
    }
    
    // Restore from backup
    private static bool RestoreFromBackup()
    {
        GetTraderXLogger().LogWarning("Attempting to restore from backup...");
        
        string productsBackup = TRADERX_COMPILED_PRODUCTS_BACKUP;
        string categoriesBackup = TRADERX_COMPILED_CATEGORIES_BACKUP;
        
        if (!FileExist(productsBackup) || !FileExist(categoriesBackup))
        {
            GetTraderXLogger().LogError("No backup available! Server may fail to start.");
            return false;
        }
        
        string productsTarget = TRADERX_COMPILED_PRODUCTS_FILE;
        string categoriesTarget = TRADERX_COMPILED_CATEGORIES_FILE;
        
        CopyFile(productsBackup, productsTarget);
        CopyFile(categoriesBackup, categoriesTarget);
        
        GetTraderXLogger().LogInfo("Restored from backup successfully. Server will use last known good configuration.");
        return false;
    }
    
    // Helper: Sort packs by priority (alphabetical)
    private static void SortPacksByPriority(array<ref TraderXCsvPackInfo> packs)
    {
        // Simple bubble sort by filename (alphabetical = priority)
        for (int i = 0; i < packs.Count() - 1; i++)
        {
            for (int j = 0; j < packs.Count() - i - 1; j++)
            {
                if (packs.Get(j).fileName > packs.Get(j + 1).fileName)
                {
                    TraderXCsvPackInfo temp = packs.Get(j);
                    packs.Set(j, packs.Get(j + 1));
                    packs.Set(j + 1, temp);
                }
            }
        }
    }
    
    // Helper: Merge products (later overrides earlier)
    private static void MergeProducts(map<string, ref TraderXCsvProduct> productMap, array<ref TraderXCsvProduct> newProducts)
    {
        foreach (TraderXCsvProduct product : newProducts)
        {
            if (product.productKey.Length() > 0)
            {
                productMap.Set(product.productKey, product);
            }
        }
    }
    
    // Helper: Merge categories (later overrides earlier)
    private static void MergeCategories(map<string, ref TraderXCsvCategory> categoryMap, array<ref TraderXCsvCategory> newCategories)
    {
        foreach (TraderXCsvCategory category : newCategories)
        {
            if (category.categoryKey.Length() > 0)
            {
                categoryMap.Set(category.categoryKey, category);
            }
        }
    }
    
    // Check current format and migrate if needed
    private static void CheckAndMigrateFormat(string preferredFormat)
    {
        preferredFormat.ToUpper();
        
        string filename;
        FileAttr attr;
        
        // Check what format currently exists
        bool hasCsv = false;
        bool hasJson = false;
        
        FindFileHandle csvHandle = FindFile(TRADERX_CSV_SOURCE_DIR + "*.csv", filename, attr, FindFileFlags.ALL);
        if (csvHandle != 0)
        {
            hasCsv = true;
            CloseFindFile(csvHandle);
        }
        
        FindFileHandle jsonHandle = FindFile(TRADERX_CSV_SOURCE_DIR + "*.json", filename, attr, FindFileFlags.ALL);
        if (jsonHandle != 0)
        {
            hasJson = true;
            CloseFindFile(jsonHandle);
        }
        
        // Determine if migration is needed
        if (preferredFormat == "CSV" && hasJson && !hasCsv)
        {
            GetTraderXLogger().LogInfo("Preferred format is CSV but only JSON found - migrating JSON to CSV");
            MigrateJsonToCsv();
        }
        else if (preferredFormat == "JSON" && hasCsv && !hasJson)
        {
            GetTraderXLogger().LogInfo("Preferred format is JSON but only CSV found - migrating CSV to JSON");
            MigrateCsvToJson();
        }
        else if (hasCsv && hasJson)
        {
            GetTraderXLogger().LogWarning("Both CSV and JSON files found in Source - using auto-detection (CSV takes priority)");
        }
    }
    
    // Migrate JSON source files to CSV format
    private static void MigrateJsonToCsv()
    {
        GetTraderXLogger().LogInfo("Starting JSON to CSV migration...");
        
        ref array<ref TraderXCsvProduct> products = new array<ref TraderXCsvProduct>;
        ref array<ref TraderXCsvCategory> categories = new array<ref TraderXCsvCategory>;
        ref array<ref TraderXCsvPreset> presets = new array<ref TraderXCsvPreset>;
        
        // Load JSON files
        string productsJsonPath = TRADERX_CSV_SOURCE_DIR + "products.json";
        if (FileExist(productsJsonPath))
        {
            ref array<ref TraderXJsonProduct> jsonProducts = new array<ref TraderXJsonProduct>;
            JsonFileLoader<array<ref TraderXJsonProduct>>.JsonLoadFile(productsJsonPath, jsonProducts);
            
            if (jsonProducts)
            {
                foreach (TraderXJsonProduct jsonProd : jsonProducts)
                {
                    TraderXCsvProduct csvProd = ConvertJsonProductToCsv(jsonProd);
                    if (csvProd)
                    {
                        products.Insert(csvProd);
                    }
                }
            }
        }
        
        string categoriesJsonPath = TRADERX_CSV_SOURCE_DIR + "categories.json";
        if (FileExist(categoriesJsonPath))
        {
            ref array<ref TraderXJsonCategory> jsonCategories = new array<ref TraderXJsonCategory>;
            JsonFileLoader<array<ref TraderXJsonCategory>>.JsonLoadFile(categoriesJsonPath, jsonCategories);
            
            if (jsonCategories)
            {
                foreach (TraderXJsonCategory jsonCat : jsonCategories)
                {
                    TraderXCsvCategory csvCat = ConvertJsonCategoryToCsv(jsonCat);
                    if (csvCat)
                    {
                        categories.Insert(csvCat);
                    }
                }
            }
        }
        
        // Export to CSV
        if (products.Count() > 0)
        {
            TraderXJsonToCsvConverter.ExportProductsToCSV(products, TRADERX_CSV_SOURCE_DIR + "products.csv");
            GetTraderXLogger().LogInfo(string.Format("Migrated %1 products to CSV", products.Count()));
        }
        
        if (categories.Count() > 0)
        {
            TraderXJsonToCsvConverter.ExportCategoriesToCSV(categories, TRADERX_CSV_SOURCE_DIR + "categories.csv");
            GetTraderXLogger().LogInfo(string.Format("Migrated %1 categories to CSV", categories.Count()));
        }
        
        GetTraderXLogger().LogInfo("JSON to CSV migration complete");
    }
    
    // Migrate CSV source files to JSON format
    private static void MigrateCsvToJson()
    {
        GetTraderXLogger().LogInfo("Starting CSV to JSON migration...");
        
        ref array<ref TraderXCsvProduct> products = new array<ref TraderXCsvProduct>;
        ref array<ref TraderXCsvCategory> categories = new array<ref TraderXCsvCategory>;
        ref array<ref TraderXCsvPreset> presets = new array<ref TraderXCsvPreset>;
        
        // Load CSV files
        bool loadSuccess = LoadCsvPacks(products, categories, presets);
        
        if (!loadSuccess)
        {
            GetTraderXLogger().LogError("Failed to load CSV files for migration");
            return;
        }
        
        // Convert to JSON format
        ref array<ref TraderXCompiledProduct> jsonProducts = new array<ref TraderXCompiledProduct>;
        
        foreach (TraderXCsvProduct csvProd : products)
        {
            TraderXCompiledProduct jsonProd = new TraderXCompiledProduct();
            jsonProd.productId = csvProd.productKey;
            jsonProd.className = csvProd.className;
            jsonProd.buyPrice = csvProd.buyPrice;
            jsonProd.sellPrice = csvProd.sellPrice;
            jsonProd.maxStock = csvProd.maxStock;
            jsonProd.coefficient = csvProd.destockCoefficient;
            jsonProd.tradeQuantity = PackTradeQuantity(csvProd.buyQtyMode, csvProd.buyQtyValue, csvProd.sellQtyMode, csvProd.sellQtyValue);
            jsonProd.stockSettings = PackStockSettings(csvProd.destockCoefficient, csvProd.stockBehaviorAtRestart);
            jsonProd.attachments = csvProd.attachments;
            jsonProd.variants = csvProd.variants;
            jsonProducts.Insert(jsonProd);
        }
        
        ref array<ref TraderXCompiledCategory> jsonCategories = new array<ref TraderXCompiledCategory>;
        
        foreach (TraderXCsvCategory csvCat : categories)
        {
            TraderXCompiledCategory jsonCat = new TraderXCompiledCategory();
            jsonCat.categoryId = csvCat.categoryKey;
            jsonCat.categoryName = csvCat.categoryName;
            jsonCat.icon = csvCat.icon;
            jsonCat.isVisible = csvCat.isVisible;
            jsonCat.licensesRequired = csvCat.licensesRequired;
            jsonCat.productIds = csvCat.productKeys;
            jsonCategories.Insert(jsonCat);
        }
        
        // Save as JSON
        if (jsonProducts.Count() > 0)
        {
            JsonFileLoader<array<ref TraderXCompiledProduct>>.JsonSaveFile(TRADERX_CSV_SOURCE_DIR + "products.json", jsonProducts);
            GetTraderXLogger().LogInfo(string.Format("Migrated %1 products to JSON", jsonProducts.Count()));
        }
        
        if (jsonCategories.Count() > 0)
        {
            JsonFileLoader<array<ref TraderXCompiledCategory>>.JsonSaveFile(TRADERX_CSV_SOURCE_DIR + "categories.json", jsonCategories);
            GetTraderXLogger().LogInfo(string.Format("Migrated %1 categories to JSON", jsonCategories.Count()));
        }
        
        GetTraderXLogger().LogInfo("CSV to JSON migration complete");
    }
    
    // Helper: Merge presets (later overrides earlier)
    private static void MergePresets(map<string, ref TraderXCsvPreset> presetMap, array<ref TraderXCsvPreset> newPresets)
    {
        foreach (TraderXCsvPreset preset : newPresets)
        {
            if (preset.presetKey.Length() > 0)
            {
                presetMap.Set(preset.presetKey, preset);
            }
        }
    }
}
