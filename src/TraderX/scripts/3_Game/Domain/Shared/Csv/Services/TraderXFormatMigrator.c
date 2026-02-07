/**
 * Domain Service: TraderXFormatMigrator
 * 
 * Responsibility: Migrate configuration data between CSV and JSON formats.
 * 
 * This service handles the conversion of configuration files from one format
 * to another, ensuring data integrity through bitfield packing/unpacking and
 * validation. Migrations are non-destructive - original files are preserved.
 * 
 * Supported Migrations:
 * - JSON → CSV: Unpacks bitfields to human-readable CSV columns
 * - CSV → JSON: Packs CSV columns into compact bitfield integers
 * 
 * @author TraderX Team
 * @since 2.0 (CSV System)
 */
class TraderXFormatMigrator
{
    private string m_sourceDirectory;
    
    /**
     * Constructor
     * 
     * @param sourceDirectory Full path to the source directory
     */
    void TraderXFormatMigrator(string sourceDirectory)
    {
        m_sourceDirectory = sourceDirectory;
    }
    
    /**
     * Migrate source files to the specified target format.
     * 
     * @param targetFormat Target format ("CSV" or "JSON")
     * @return true if migration succeeded, false otherwise
     */
    bool MigrateToFormat(string targetFormat)
    {
        targetFormat.ToUpper();
        
        if (targetFormat == "CSV")
        {
            return MigrateJsonToCsv();
        }
        else if (targetFormat == "JSON")
        {
            return MigrateCsvToJson();
        }
        
        GetTraderXLogger().LogError("Invalid target format: " + targetFormat);
        return false;
    }
    
    /**
     * Migrate JSON source files to CSV format.
     * 
     * Process:
     * 1. Load products.json and categories.json
     * 2. Unpack bitfields to CSV model
     * 3. Export to products.csv and categories.csv
     * 
     * @return true if migration succeeded
     */
    private bool MigrateJsonToCsv()
    {
        GetTraderXLogger().LogInfo("Starting JSON to CSV migration...");
        
        ref array<ref TraderXCsvProduct> products = new array<ref TraderXCsvProduct>;
        ref array<ref TraderXCsvCategory> categories = new array<ref TraderXCsvCategory>;
        
        // Load JSON files
        string productsJsonPath = m_sourceDirectory + "products.json";
        if (FileExist(productsJsonPath))
        {
            ref array<ref TraderXCompiledProduct> jsonProducts = new array<ref TraderXCompiledProduct>;
            JsonFileLoader<array<ref TraderXCompiledProduct>>.JsonLoadFile(productsJsonPath, jsonProducts);
            
            if (jsonProducts)
            {
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
        
        string categoriesJsonPath = m_sourceDirectory + "categories.json";
        if (FileExist(categoriesJsonPath))
        {
            ref array<ref TraderXCompiledCategory> jsonCategories = new array<ref TraderXCompiledCategory>;
            JsonFileLoader<array<ref TraderXCompiledCategory>>.JsonLoadFile(categoriesJsonPath, jsonCategories);
            
            if (jsonCategories)
            {
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
        
        // Export to CSV
        if (products.Count() > 0)
        {
            TraderXJsonToCsvConverter.ExportProductsToCSV(products, m_sourceDirectory + "products.csv");
            GetTraderXLogger().LogInfo(string.Format("Migrated %1 products to CSV", products.Count()));
        }
        
        if (categories.Count() > 0)
        {
            TraderXJsonToCsvConverter.ExportCategoriesToCSV(categories, m_sourceDirectory + "categories.csv");
            GetTraderXLogger().LogInfo(string.Format("Migrated %1 categories to CSV", categories.Count()));
        }
        
        GetTraderXLogger().LogInfo("JSON to CSV migration complete");
        return products.Count() > 0 || categories.Count() > 0;
    }
    
    /**
     * Migrate CSV source files to JSON format.
     * 
     * Process:
     * 1. Load all CSV files (merges multiple packs)
     * 2. Pack CSV fields into bitfields
     * 3. Export to products.json and categories.json
     * 
     * @return true if migration succeeded
     */
    private bool MigrateCsvToJson()
    {
        GetTraderXLogger().LogInfo("Starting CSV to JSON migration...");
        
        ref array<ref TraderXCsvProduct> products = new array<ref TraderXCsvProduct>;
        ref array<ref TraderXCsvCategory> categories = new array<ref TraderXCsvCategory>;
        ref array<ref TraderXCsvPreset> presets = new array<ref TraderXCsvPreset>;
        
        // Load CSV files using existing loader
        TraderXCsvLoader csvLoader = new TraderXCsvLoader();
        
        string filename;
        FileAttr attr;
        FindFileHandle findHandle = FindFile(m_sourceDirectory + "*.csv", filename, attr, FindFileFlags.ALL);
        
        if (findHandle == 0)
        {
            GetTraderXLogger().LogError("No CSV files found for migration");
            return false;
        }
        
        ref array<string> csvFiles = new array<string>;
        csvFiles.Insert(filename);
        
        while (FindNextFile(findHandle, filename, attr))
        {
            csvFiles.Insert(filename);
        }
        CloseFindFile(findHandle);
        
        // Sort files alphabetically for consistent merge order
        csvFiles.Sort();
        
        // Load each CSV file
        foreach (string csvFile : csvFiles)
        {
            string fullPath = m_sourceDirectory + csvFile;
            
            if (csvFile.IndexOf("product") != -1)
            {
                ref array<ref TraderXCsvProduct> packProducts = csvLoader.LoadProductsCsv(fullPath);
                if (packProducts)
                {
                    foreach (TraderXCsvProduct prod : packProducts)
                    {
                        products.Insert(prod);
                    }
                }
            }
            else if (csvFile.IndexOf("categor") != -1)
            {
                ref array<ref TraderXCsvCategory> packCategories = csvLoader.LoadCategoriesCsv(fullPath);
                if (packCategories)
                {
                    foreach (TraderXCsvCategory cat : packCategories)
                    {
                        categories.Insert(cat);
                    }
                }
            }
        }
        
        // Convert to JSON format
        ref array<ref TraderXCompiledProduct> jsonProducts = new array<ref TraderXCompiledProduct>;
        
        foreach (TraderXCsvProduct csvProd : products)
        {
            TraderXCompiledProduct jsonProd = ConvertCsvProductToJson(csvProd);
            if (jsonProd)
            {
                jsonProducts.Insert(jsonProd);
            }
        }
        
        ref array<ref TraderXCompiledCategory> jsonCategories = new array<ref TraderXCompiledCategory>;
        
        foreach (TraderXCsvCategory csvCat : categories)
        {
            TraderXCompiledCategory jsonCat = ConvertCsvCategoryToJson(csvCat);
            if (jsonCat)
            {
                jsonCategories.Insert(jsonCat);
            }
        }
        
        // Save as JSON
        if (jsonProducts.Count() > 0)
        {
            JsonFileLoader<array<ref TraderXCompiledProduct>>.JsonSaveFile(m_sourceDirectory + "products.json", jsonProducts);
            GetTraderXLogger().LogInfo(string.Format("Migrated %1 products to JSON", jsonProducts.Count()));
        }
        
        if (jsonCategories.Count() > 0)
        {
            JsonFileLoader<array<ref TraderXCompiledCategory>>.JsonSaveFile(m_sourceDirectory + "categories.json", jsonCategories);
            GetTraderXLogger().LogInfo(string.Format("Migrated %1 categories to JSON", jsonCategories.Count()));
        }
        
        GetTraderXLogger().LogInfo("CSV to JSON migration complete");
        return jsonProducts.Count() > 0 || jsonCategories.Count() > 0;
    }
    
    /**
     * Convert JSON product to CSV model (unpack bitfields).
     * 
     * @param jsonProd JSON product with packed bitfields
     * @return CSV product with unpacked fields
     */
    private TraderXCsvProduct ConvertJsonProductToCsv(TraderXCompiledProduct jsonProd)
    {
        TraderXCsvProduct csvProd = new TraderXCsvProduct();
        
        csvProd.productKey = jsonProd.productId;
        csvProd.className = jsonProd.className;
        csvProd.buyPrice = jsonProd.buyPrice;
        csvProd.sellPrice = jsonProd.sellPrice;
        csvProd.maxStock = jsonProd.maxStock;
        
        // Unpack tradeQuantity bitfield using service
        string buyMode;
        float buyValue;
        string sellMode;
        float sellValue;
        
        TraderXBitfieldPacker.UnpackTradeQuantity(jsonProd.tradeQuantity, buyMode, buyValue, sellMode, sellValue);
        
        csvProd.buyQtyMode = buyMode;
        csvProd.buyQtyValue = buyValue;
        csvProd.sellQtyMode = sellMode;
        csvProd.sellQtyValue = sellValue;
        
        // Unpack stockSettings bitfield using service
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
    
    /**
     * Convert JSON category to CSV model.
     * 
     * @param jsonCat JSON category
     * @return CSV category
     */
    private TraderXCsvCategory ConvertJsonCategoryToCsv(TraderXCompiledCategory jsonCat)
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
    
    /**
     * Convert CSV product to JSON model (pack bitfields).
     * 
     * @param csvProd CSV product with unpacked fields
     * @return JSON product with packed bitfields
     */
    private TraderXCompiledProduct ConvertCsvProductToJson(TraderXCsvProduct csvProd)
    {
        TraderXCompiledProduct jsonProd = new TraderXCompiledProduct();
        
        jsonProd.productId = csvProd.productKey;
        jsonProd.className = csvProd.className;
        jsonProd.buyPrice = csvProd.buyPrice;
        jsonProd.sellPrice = csvProd.sellPrice;
        jsonProd.maxStock = csvProd.maxStock;
        jsonProd.coefficient = csvProd.destockCoefficient;
        
        // Pack bitfields using service
        jsonProd.tradeQuantity = TraderXBitfieldPacker.PackTradeQuantity(csvProd.buyQtyMode, csvProd.buyQtyValue, csvProd.sellQtyMode, csvProd.sellQtyValue);
        
        jsonProd.stockSettings = TraderXBitfieldPacker.PackStockSettings(csvProd.destockCoefficient, csvProd.stockBehaviorAtRestart);
        
        // Copy arrays
        jsonProd.attachments = csvProd.attachments;
        jsonProd.variants = csvProd.variants;
        
        return jsonProd;
    }
    
    /**
     * Convert CSV category to JSON model.
     * 
     * @param csvCat CSV category
     * @return JSON category
     */
    private TraderXCompiledCategory ConvertCsvCategoryToJson(TraderXCsvCategory csvCat)
    {
        TraderXCompiledCategory jsonCat = new TraderXCompiledCategory();
        
        jsonCat.categoryId = csvCat.categoryKey;
        jsonCat.categoryName = csvCat.categoryName;
        jsonCat.icon = csvCat.icon;
        jsonCat.isVisible = csvCat.isVisible;
        jsonCat.licensesRequired = csvCat.licensesRequired;
        jsonCat.productIds = csvCat.productKeys;
        
        return jsonCat;
    }
}
