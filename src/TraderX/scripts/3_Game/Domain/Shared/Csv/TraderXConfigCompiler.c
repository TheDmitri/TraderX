/**
 * TraderX Configuration Compiler: Source/ (CSV or JSON arrays) -> Compiled/*_compiled.json
 *
 * 1. Load every Source/*.csv in alphabetical order; the file name decides the type
 *    ("product", "categor" or "preset" in the name). A later file overrides rows of an earlier
 *    one with the same key, so "00_imported_*.csv" packs lose against the admin's own files.
 *    Without CSV files, Source/products.json + categories.json (arrays of compiled models) are read.
 * 2. Validate (errors abort, see TraderXLogs/Config/ConfigReport.log).
 * 3. Write Compiled/Products_compiled.json, Categories_compiled.json and TraderXPresets/presets_<id>.json.
 * 4. Copy the compiled files to Backup/; on a failed compile the backup is restored (last good catalog).
 */
class TraderXConfigCompiler
{
    static bool CompileAndValidate()
    {
        GetTraderXLogger().LogInfo("=== TraderX Configuration Compilation Started ===");

        if (!FileExist(TRADERX_CSV_SOURCE_DIR))
            MakeDirectory(TRADERX_CSV_SOURCE_DIR);
        if (!FileExist(TRADERX_COMPILED_DIR))
            MakeDirectory(TRADERX_COMPILED_DIR);
        if (!FileExist(TRADERX_BACKUP_DIR))
            MakeDirectory(TRADERX_BACKUP_DIR);

        TraderXConfigValidator validator = new TraderXConfigValidator();
        array<ref TraderXCsvProduct> products = new array<ref TraderXCsvProduct>;
        array<ref TraderXCsvCategory> categories = new array<ref TraderXCsvCategory>;
        array<ref TraderXCsvPreset> presets = new array<ref TraderXCsvPreset>;

        if (!LoadSourceFiles(products, categories, presets, validator))
        {
            GetTraderXLogger().LogError("Failed to load source files");
            return RestoreFromBackup();
        }

        if (products.Count() == 0)
        {
            GetTraderXLogger().LogWarning("[TraderX] Source/ has no product rows yet: nothing to compile. Fill Source/products.csv and Source/categories.csv, then restart.");
            return false;
        }

        validator.ValidateProducts(products);
        validator.ValidateCategories(categories, products);
        validator.ValidatePresets(presets, products);
        validator.ValidateCrossReferences(products);
        validator.GenerateReport(TRADERX_CONFIG_REPORT_FILE);

        if (!validator.IsValid())
        {
            GetTraderXLogger().LogError(string.Format("Configuration validation failed with %1 errors. See report: %2", validator.GetErrorCount(), TRADERX_CONFIG_REPORT_FILE));
            return RestoreFromBackup();
        }

        GetTraderXLogger().LogInfo(string.Format("Validation passed. Errors: 0, Warnings: %1", validator.GetWarningCount()));

        CompileToJson(products, categories, presets);
        CreateBackup();

        GetTraderXLogger().LogInfo("=== TraderX Configuration Compilation Complete ===");
        return true;
    }

    static bool HasSourceFiles()
    {
        array<string> csvFiles = TraderXCsvLoader.ListFiles(TRADERX_CSV_SOURCE_DIR, "csv");
        if (csvFiles.Count() > 0)
            return true;
        return FileExist(TRADERX_CSV_SOURCE_DIR + "products.json");
    }

    private static bool LoadSourceFiles(array<ref TraderXCsvProduct> products, array<ref TraderXCsvCategory> categories, array<ref TraderXCsvPreset> presets, TraderXConfigValidator validator)
    {
        array<string> csvFiles = TraderXCsvLoader.ListFiles(TRADERX_CSV_SOURCE_DIR, "csv");
        if (csvFiles.Count() > 0)
        {
            if (FileExist(TRADERX_CSV_SOURCE_DIR + "products.json"))
                GetTraderXLogger().LogWarning("Both CSV and JSON files found in Source - CSV takes priority, products.json is ignored");
            return LoadCsvPacks(csvFiles, products, categories, presets, validator);
        }

        if (FileExist(TRADERX_CSV_SOURCE_DIR + "products.json"))
            return LoadJsonSource(products, categories);

        GetTraderXLogger().LogError("No CSV or JSON source files found in " + TRADERX_CSV_SOURCE_DIR);
        return false;
    }

    private static bool LoadCsvPacks(array<string> csvFiles, array<ref TraderXCsvProduct> products, array<ref TraderXCsvCategory> categories, array<ref TraderXCsvPreset> presets, TraderXConfigValidator validator)
    {
        GetTraderXLogger().LogInfo(string.Format("Loading %1 CSV file(s) from: %2", csvFiles.Count(), TRADERX_CSV_SOURCE_DIR));

        map<string, ref TraderXCsvProduct> productMap = new map<string, ref TraderXCsvProduct>;
        map<string, ref TraderXCsvCategory> categoryMap = new map<string, ref TraderXCsvCategory>;
        map<string, ref TraderXCsvPreset> presetMap = new map<string, ref TraderXCsvPreset>;
        array<string> productOrder = new array<string>;
        array<string> categoryOrder = new array<string>;
        array<string> presetOrder = new array<string>;

        foreach (string fileName : csvFiles)
        {
            string filePath = TRADERX_CSV_SOURCE_DIR + fileName;
            string lowerName = fileName;
            lowerName.ToLower();

            if (lowerName.Contains("product"))
            {
                array<ref TraderXCsvProduct> packProducts = TraderXCsvLoader.LoadProductsCsv(filePath);
                foreach (TraderXCsvProduct product : packProducts)
                {
                    if (product.productKey == "")
                        continue;
                    TraderXCsvProduct previousProduct = productMap.Get(product.productKey);
                    if (previousProduct && previousProduct.sourceFile == product.sourceFile)
                        validator.AddError(product.sourceFile, product.sourceRow, "productKey", string.Format("Duplicate productKey (first at line %1)", previousProduct.sourceRow), product.productKey);
                    else if (!previousProduct)
                        productOrder.Insert(product.productKey);
                    productMap.Set(product.productKey, product);
                }
            }
            else if (lowerName.Contains("categor"))
            {
                array<ref TraderXCsvCategory> packCategories = TraderXCsvLoader.LoadCategoriesCsv(filePath);
                foreach (TraderXCsvCategory category : packCategories)
                {
                    if (category.categoryKey == "")
                        continue;
                    TraderXCsvCategory previousCategory = categoryMap.Get(category.categoryKey);
                    if (previousCategory && previousCategory.sourceFile == category.sourceFile)
                        validator.AddError(category.sourceFile, category.sourceRow, "categoryKey", string.Format("Duplicate categoryKey (first at line %1)", previousCategory.sourceRow), category.categoryKey);
                    else if (!previousCategory)
                        categoryOrder.Insert(category.categoryKey);
                    categoryMap.Set(category.categoryKey, category);
                }
            }
            else if (lowerName.Contains("preset"))
            {
                array<ref TraderXCsvPreset> packPresets = TraderXCsvLoader.LoadPresetsCsv(filePath);
                foreach (TraderXCsvPreset preset : packPresets)
                {
                    if (preset.presetKey == "")
                        continue;
                    TraderXCsvPreset previousPreset = presetMap.Get(preset.presetKey);
                    if (previousPreset && previousPreset.sourceFile == preset.sourceFile)
                        validator.AddError(preset.sourceFile, preset.sourceRow, "presetKey", string.Format("Duplicate presetKey (first at line %1)", previousPreset.sourceRow), preset.presetKey);
                    else if (!previousPreset)
                        presetOrder.Insert(preset.presetKey);
                    presetMap.Set(preset.presetKey, preset);
                }
            }
            else
            {
                GetTraderXLogger().LogWarning("Unknown CSV file type (name should contain 'product', 'categor', or 'preset'): " + fileName);
            }
        }

        foreach (string productKey : productOrder)
        {
            products.Insert(productMap.Get(productKey));
        }
        foreach (string categoryKey : categoryOrder)
        {
            categories.Insert(categoryMap.Get(categoryKey));
        }
        foreach (string presetKey : presetOrder)
        {
            presets.Insert(presetMap.Get(presetKey));
        }

        GetTraderXLogger().LogInfo(string.Format("Loaded totals - Products: %1, Categories: %2, Presets: %3", products.Count(), categories.Count(), presets.Count()));
        return true;
    }

    // Source/products.json + Source/categories.json: arrays in the compiled format (productId/categoryId + runtime fields)
    private static bool LoadJsonSource(array<ref TraderXCsvProduct> products, array<ref TraderXCsvCategory> categories)
    {
        GetTraderXLogger().LogInfo("Loading JSON source files from: " + TRADERX_CSV_SOURCE_DIR);

        array<ref TraderXCompiledProduct> jsonProducts = new array<ref TraderXCompiledProduct>;
        JsonFileLoader<array<ref TraderXCompiledProduct>>.JsonLoadFile(TRADERX_CSV_SOURCE_DIR + "products.json", jsonProducts);
        foreach (TraderXCompiledProduct jsonProduct : jsonProducts)
        {
            if (!jsonProduct)
                continue;
            TraderXCsvProduct csvProduct = TraderXCsvMapper.FromProduct(jsonProduct.productId, jsonProduct);
            csvProduct.sourceFile = "products.json";
            products.Insert(csvProduct);
        }

        string categoriesPath = TRADERX_CSV_SOURCE_DIR + "categories.json";
        if (FileExist(categoriesPath))
        {
            array<ref TraderXCompiledCategory> jsonCategories = new array<ref TraderXCompiledCategory>;
            JsonFileLoader<array<ref TraderXCompiledCategory>>.JsonLoadFile(categoriesPath, jsonCategories);
            foreach (TraderXCompiledCategory jsonCategory : jsonCategories)
            {
                if (!jsonCategory)
                    continue;
                TraderXCsvCategory csvCategory = TraderXCsvMapper.FromCategory(jsonCategory.categoryId, jsonCategory);
                csvCategory.sourceFile = "categories.json";
                categories.Insert(csvCategory);
            }
        }

        GetTraderXLogger().LogInfo(string.Format("Loaded totals - Products: %1, Categories: %2", products.Count(), categories.Count()));
        return true;
    }

    private static void CompileToJson(array<ref TraderXCsvProduct> products, array<ref TraderXCsvCategory> categories, array<ref TraderXCsvPreset> presets)
    {
        array<ref TraderXCompiledProduct> jsonProducts = new array<ref TraderXCompiledProduct>;
        foreach (TraderXCsvProduct csvProduct : products)
        {
            jsonProducts.Insert(TraderXCsvMapper.ToCompiledProduct(csvProduct));
        }

        array<ref TraderXCompiledCategory> jsonCategories = new array<ref TraderXCompiledCategory>;
        foreach (TraderXCsvCategory csvCategory : categories)
        {
            jsonCategories.Insert(TraderXCsvMapper.ToCompiledCategory(csvCategory));
        }

        GetTraderXLogger().LogInfo("Saving compiled products to: " + TRADERX_COMPILED_PRODUCTS_FILE);
        JsonFileLoader<array<ref TraderXCompiledProduct>>.JsonSaveFile(TRADERX_COMPILED_PRODUCTS_FILE, jsonProducts);
        GetTraderXLogger().LogInfo("Saving compiled categories to: " + TRADERX_COMPILED_CATEGORIES_FILE);
        JsonFileLoader<array<ref TraderXCompiledCategory>>.JsonSaveFile(TRADERX_COMPILED_CATEGORIES_FILE, jsonCategories);

        if (presets.Count() > 0)
            CompilePresets(presets);
    }

    // One TraderXPresets/presets_<productId>.json per product that has preset rows (overwrites that file)
    private static void CompilePresets(array<ref TraderXCsvPreset> presets)
    {
        map<string, ref TraderXPresets> presetsByProduct = new map<string, ref TraderXPresets>;
        array<string> productOrder = new array<string>;

        foreach (TraderXCsvPreset csvPreset : presets)
        {
            TraderXPresets productPresets = presetsByProduct.Get(csvPreset.productKey);
            if (!productPresets)
            {
                productPresets = new TraderXPresets();
                productPresets.productId = csvPreset.productKey;
                presetsByProduct.Set(csvPreset.productKey, productPresets);
                productOrder.Insert(csvPreset.productKey);
            }

            TraderXPreset preset = new TraderXPreset();
            preset.presetName = csvPreset.presetName;
            preset.presetId = csvPreset.presetKey;
            preset.productId = csvPreset.productKey;
            preset.attachments = TraderXCsvMapper.CopyList(csvPreset.attachments);
            productPresets.presets.Insert(preset);

            if (csvPreset.isDefault || productPresets.defaultPresetId == "")
                productPresets.defaultPresetId = csvPreset.presetKey;
        }

        if (!FileExist(TRADERX_PRESETS))
            MakeDirectory(TRADERX_PRESETS);

        foreach (string productId : productOrder)
        {
            string presetPath = string.Format(TRADERX_PRESETS_FILE, productId);
            string errorMessage;
            if (!JsonFileLoader<TraderXPresets>.SaveFile(presetPath, presetsByProduct.Get(productId), errorMessage))
                GetTraderXLogger().LogError("Failed to save preset: " + presetPath + " - " + errorMessage);
        }
        GetTraderXLogger().LogInfo(string.Format("Compiled presets for %1 products", productOrder.Count()));
    }

    private static void CreateBackup()
    {
        if (FileExist(TRADERX_COMPILED_PRODUCTS_FILE))
            CopyFile(TRADERX_COMPILED_PRODUCTS_FILE, TRADERX_COMPILED_PRODUCTS_BACKUP);
        if (FileExist(TRADERX_COMPILED_CATEGORIES_FILE))
            CopyFile(TRADERX_COMPILED_CATEGORIES_FILE, TRADERX_COMPILED_CATEGORIES_BACKUP);

        FileHandle timestampFile = OpenFile(TRADERX_BACKUP_DIR + ".backup_timestamp", FileMode.WRITE);
        if (timestampFile)
        {
            FPrintln(timestampFile, "Backup created at server startup");
            CloseFile(timestampFile);
        }
        GetTraderXLogger().LogInfo("Backup of the compiled catalog created");
    }

    // Puts the last good compiled catalog back. Always returns false: this boot's sources were not compiled.
    private static bool RestoreFromBackup()
    {
        if (!FileExist(TRADERX_COMPILED_PRODUCTS_BACKUP) || !FileExist(TRADERX_COMPILED_CATEGORIES_BACKUP))
        {
            GetTraderXLogger().LogError("No compiled backup available: the server keeps the current Compiled/ files (if any).");
            return false;
        }

        CopyFile(TRADERX_COMPILED_PRODUCTS_BACKUP, TRADERX_COMPILED_PRODUCTS_FILE);
        CopyFile(TRADERX_COMPILED_CATEGORIES_BACKUP, TRADERX_COMPILED_CATEGORIES_FILE);
        GetTraderXLogger().LogWarning("Restored the last good compiled catalog from Backup/. Fix the errors in ConfigReport.log and restart.");
        return false;
    }
}
