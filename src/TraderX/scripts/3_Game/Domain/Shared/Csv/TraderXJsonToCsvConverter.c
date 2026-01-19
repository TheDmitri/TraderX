// TraderX JSON to CSV Converter - Migrates existing JSON configs to CSV format
class TraderXJsonToCsvConverter
{
    // Main conversion entry point
    static void ConvertAllJsonToCsv()
    {
        GetTraderXLogger().LogInfo("=== Starting JSON to CSV Migration ===");
        
        // Ensure CSV source directory exists
        if (!FileExist(TRADERX_CSV_SOURCE_DIR))
        {
            MakeDirectory(TRADERX_CSV_SOURCE_DIR);
        }
        
        // Convert products
        int productCount = ConvertProductsToCSV();
        GetTraderXLogger().LogInfo(string.Format("Converted %1 products to CSV", productCount));
        
        // Convert categories
        int categoryCount = ConvertCategoriesToCSV();
        GetTraderXLogger().LogInfo(string.Format("Converted %1 categories to CSV", categoryCount));
        
        GetTraderXLogger().LogInfo("=== JSON to CSV Migration Complete ===");
        GetTraderXLogger().LogInfo("CSV files saved to: " + TRADERX_CSV_SOURCE_DIR);
        GetTraderXLogger().LogInfo("You can now restart the server to use CSV configuration");
    }
    
    // Convert products from JSON to CSV
    private static int ConvertProductsToCSV()
    {
        GetTraderXLogger().LogInfo("Converting products from JSON to CSV...");
        
        ref array<ref TraderXCsvProduct> csvProducts = new array<ref TraderXCsvProduct>;
        
        // Scan products directory
        if (!FileExist(TRADERX_PRODUCTS_DIR))
        {
            GetTraderXLogger().LogWarning("Products directory not found: " + TRADERX_PRODUCTS_DIR);
            return 0;
        }
        
        string filename;
        FileAttr attr;
        FindFileHandle findHandle = FindFile(TRADERX_PRODUCTS_DIR + "*.json", filename, attr, FindFileFlags.ALL);
        
        if (findHandle == 0)
        {
            GetTraderXLogger().LogWarning("No product JSON files found");
            CloseFindFile(findHandle);
            return 0;
        }
        
        // Process first file
        if (TraderXProductId.IsValidUUID(filename))
        {
            TraderXCsvProduct csvProd = ConvertProductJsonToCsv(TRADERX_PRODUCTS_DIR + filename, filename);
            if (csvProd)
            {
                csvProducts.Insert(csvProd);
            }
        }
        
        // Process remaining files
        while (FindNextFile(findHandle, filename, attr))
        {
            if (TraderXProductId.IsValidUUID(filename))
            {
                TraderXCsvProduct csvProd2 = ConvertProductJsonToCsv(TRADERX_PRODUCTS_DIR + filename, filename);
                if (csvProd2)
                {
                    csvProducts.Insert(csvProd2);
                }
            }
        }
        
        CloseFindFile(findHandle);
        
        // Export to CSV
        if (csvProducts.Count() > 0)
        {
            ExportProductsToCSV(csvProducts, TRADERX_CSV_SOURCE_DIR + "products_migrated.csv");
        }
        
        return csvProducts.Count();
    }
    
    // Convert categories from JSON to CSV
    private static int ConvertCategoriesToCSV()
    {
        GetTraderXLogger().LogInfo("Converting categories from JSON to CSV...");
        
        ref array<ref TraderXCsvCategory> csvCategories = new array<ref TraderXCsvCategory>;
        
        // Scan categories directory
        if (!FileExist(TRADERX_CATEGORIES_DIR))
        {
            GetTraderXLogger().LogWarning("Categories directory not found: " + TRADERX_CATEGORIES_DIR);
            return 0;
        }
        
        string filename;
        FileAttr attr;
        FindFileHandle findHandle = FindFile(TRADERX_CATEGORIES_DIR + "*.json", filename, attr, FindFileFlags.ALL);
        
        if (findHandle == 0)
        {
            GetTraderXLogger().LogWarning("No category JSON files found");
            CloseFindFile(findHandle);
            return 0;
        }
        
        // Process first file
        if (TraderXCategoryId.IsValidUUID(filename))
        {
            TraderXCsvCategory csvCat = ConvertCategoryJsonToCsv(TRADERX_CATEGORIES_DIR + filename);
            if (csvCat)
            {
                csvCategories.Insert(csvCat);
            }
        }
        
        // Process remaining files
        while (FindNextFile(findHandle, filename, attr))
        {
            if (TraderXCategoryId.IsValidUUID(filename))
            {
                TraderXCsvCategory csvCat2 = ConvertCategoryJsonToCsv(TRADERX_CATEGORIES_DIR + filename);
                if (csvCat2)
                {
                    csvCategories.Insert(csvCat2);
                }
            }
        }
        
        CloseFindFile(findHandle);
        
        // Export to CSV
        if (csvCategories.Count() > 0)
        {
            ExportCategoriesToCSV(csvCategories, TRADERX_CSV_SOURCE_DIR + "categories_migrated.csv");
        }
        
        return csvCategories.Count();
    }
    
    // Convert single product JSON to CSV model
    private static TraderXCsvProduct ConvertProductJsonToCsv(string filePath, string filename)
    {
        TraderXJsonProduct jsonProduct = new TraderXJsonProduct();
        JsonFileLoader<TraderXJsonProduct>.JsonLoadFile(filePath, jsonProduct);
        
        if (!jsonProduct)
        {
            return null;
        }
        
        TraderXCsvProduct csvProduct = new TraderXCsvProduct();
        
        // Extract product key from filename
        filename.Replace(".json", "");
        csvProduct.productKey = filename;
        
        // Basic fields
        csvProduct.className = jsonProduct.className;
        csvProduct.buyPrice = jsonProduct.buyPrice;
        csvProduct.sellPrice = jsonProduct.sellPrice;
        csvProduct.maxStock = jsonProduct.maxStock;
        csvProduct.destockCoefficient = jsonProduct.coefficient;
        
        // Unpack tradeQuantity bitfield
        UnpackTradeQuantity(jsonProduct.tradeQuantity, csvProduct);
        
        // Unpack stockSettings bitfield
        UnpackStockSettings(jsonProduct.stockSettings, csvProduct);
        
        // Copy arrays
        csvProduct.attachments = jsonProduct.attachments;
        csvProduct.variants = jsonProduct.variants;
        
        return csvProduct;
    }
    
    // Convert single category JSON to CSV model
    private static TraderXCsvCategory ConvertCategoryJsonToCsv(string filePath)
    {
        TraderXJsonCategory jsonCategory = new TraderXJsonCategory();
        JsonFileLoader<TraderXJsonCategory>.JsonLoadFile(filePath, jsonCategory);
        
        if (!jsonCategory)
        {
            return null;
        }
        
        TraderXCsvCategory csvCategory = new TraderXCsvCategory();
        
        // Extract category key from filename
        string filename = filePath;
        int lastSlash = filename.LastIndexOf("\\");
        if (lastSlash >= 0)
        {
            filename = filename.Substring(lastSlash + 1, filename.Length() - lastSlash - 1);
        }
        filename.Replace(".json", "");
        csvCategory.categoryKey = filename;
        
        // Basic fields
        csvCategory.categoryName = jsonCategory.categoryName;
        csvCategory.icon = jsonCategory.icon;
        csvCategory.isVisible = jsonCategory.isVisible;
        
        // Copy arrays
        csvCategory.licensesRequired = jsonCategory.licensesRequired;
        csvCategory.productKeys = jsonCategory.productIds;
        
        return csvCategory;
    }
    
    // Unpack tradeQuantity bitfield to CSV fields
    private static void UnpackTradeQuantity(int packed, TraderXCsvProduct csvProduct)
    {
        // Extract sell mode (bits 0-2)
        int sellMode = packed & 0x7;
        switch (sellMode)
        {
            case 0: csvProduct.sellQtyMode = "NO_MATTER"; break;
            case 1: csvProduct.sellQtyMode = "EMPTY"; break;
            case 2: csvProduct.sellQtyMode = "FULL"; break;
            case 3: csvProduct.sellQtyMode = "COEFFICIENT"; break;
            case 4: csvProduct.sellQtyMode = "STATIC"; break;
            default: csvProduct.sellQtyMode = "FULL"; break;
        }
        
        // Extract buy mode (bits 3-5)
        int buyMode = (packed >> 3) & 0x7;
        switch (buyMode)
        {
            case 0: csvProduct.buyQtyMode = "EMPTY"; break;
            case 1: csvProduct.buyQtyMode = "EMPTY"; break;
            case 2: csvProduct.buyQtyMode = "FULL"; break;
            case 3: csvProduct.buyQtyMode = "COEFFICIENT"; break;
            case 4: csvProduct.buyQtyMode = "STATIC"; break;
            default: csvProduct.buyQtyMode = "FULL"; break;
        }
        
        // Extract values if needed
        if (sellMode == 3 || sellMode == 4) // COEFFICIENT or STATIC
        {
            int sellValueInt = (packed >> 6) & 0x3FF;
            csvProduct.sellQtyValue = sellValueInt / 1000.0;
        }
        
        if (buyMode == 3 || buyMode == 4) // COEFFICIENT or STATIC
        {
            int buyValueInt = (packed >> 16) & 0xFFFF;
            csvProduct.buyQtyValue = buyValueInt / 1000.0;
        }
    }
    
    // Unpack stockSettings bitfield to CSV fields
    private static void UnpackStockSettings(int packed, TraderXCsvProduct csvProduct)
    {
        // Extract destock coefficient (bits 0-15)
        int coefInt = packed & 0xFFFF;
        csvProduct.destockCoefficient = coefInt / 10000.0;
        
        // Extract stock behavior (bits 16-17)
        csvProduct.stockBehaviorAtRestart = (packed >> 16) & 0x3;
    }
    
    // Export products to CSV file
    static void ExportProductsToCSV(array<ref TraderXCsvProduct> products, string filePath)
    {
        FileHandle file = OpenFile(filePath, FileMode.WRITE);
        if (!file)
        {
            GetTraderXLogger().LogError("Failed to create CSV file: " + filePath);
            return;
        }
        
        // Write header
        FPrintln(file, "productKey\tclassName\tbuyPrice\tsellPrice\tmaxStock\tbuyQtyMode\tbuyQtyValue\tsellQtyMode\tsellQtyValue\tdestockCoefficient\tstockBehaviorAtRestart\tattachments\tvariants\tnotes");
        
        // Write data rows
        foreach (TraderXCsvProduct product : products)
        {
            string attachmentsStr = "";
            if (product.attachments)
            {
                for (int i = 0; i < product.attachments.Count(); i++)
                {
                    if (i > 0) attachmentsStr += ";";
                    attachmentsStr += product.attachments.Get(i);
                }
            }
            
            string variantsStr = "";
            if (product.variants)
            {
                for (int j = 0; j < product.variants.Count(); j++)
                {
                    if (j > 0) variantsStr += ";";
                    variantsStr += product.variants.Get(j);
                }
            }
            
            string line = product.productKey;
            line += "\t" + product.className;
            line += "\t" + product.buyPrice.ToString();
            line += "\t" + product.sellPrice.ToString();
            line += "\t" + product.maxStock.ToString();
            line += "\t" + product.buyQtyMode;
            line += "\t" + product.buyQtyValue.ToString();
            line += "\t" + product.sellQtyMode;
            line += "\t" + product.sellQtyValue.ToString();
            line += "\t" + product.destockCoefficient.ToString();
            line += "\t" + product.stockBehaviorAtRestart.ToString();
            line += "\t" + attachmentsStr;
            line += "\t" + variantsStr;
            line += "\t";
            
            FPrintln(file, line);
        }
        
        CloseFile(file);
        GetTraderXLogger().LogInfo("Exported products to: " + filePath);
    }
    
    // Export categories to CSV file
    static void ExportCategoriesToCSV(array<ref TraderXCsvCategory> categories, string filePath)
    {
        FileHandle file = OpenFile(filePath, FileMode.WRITE);
        if (!file)
        {
            GetTraderXLogger().LogError("Failed to create CSV file: " + filePath);
            return;
        }
        
        // Write header
        FPrintln(file, "categoryKey\tcategoryName\ticon\tisVisible\tlicensesRequired\tproductKeys\tnotes");
        
        // Write data rows
        foreach (TraderXCsvCategory category : categories)
        {
            string licensesStr = "";
            if (category.licensesRequired)
            {
                for (int i = 0; i < category.licensesRequired.Count(); i++)
                {
                    if (i > 0) licensesStr += ";";
                    licensesStr += category.licensesRequired.Get(i);
                }
            }
            
            string productsStr = "";
            if (category.productKeys)
            {
                for (int j = 0; j < category.productKeys.Count(); j++)
                {
                    if (j > 0) productsStr += ";";
                    productsStr += category.productKeys.Get(j);
                }
            }
            
            string visibleStr;
            if (category.isVisible)
                visibleStr = "1";
            else
                visibleStr = "0";
            
            string line = category.categoryKey;
            line += "\t" + category.categoryName;
            line += "\t" + category.icon;
            line += "\t" + visibleStr;
            line += "\t" + licensesStr;
            line += "\t" + productsStr;
            line += "\t";
            
            FPrintln(file, line);
        }
        
        CloseFile(file);
        GetTraderXLogger().LogInfo("Exported categories to: " + filePath);
    }
}
