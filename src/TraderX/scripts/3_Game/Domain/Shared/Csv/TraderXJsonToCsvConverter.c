// TraderX catalog -> CSV export.
// Exports what the repositories currently hold (loaded from the legacy Products/ and Categories/ files),
// plus the server presets of those products, as TAB-separated files the compiler reads back.
class TraderXJsonToCsvConverter
{
    static const string PRODUCTS_HEADER = "productKey\tclassName\tbuyPrice\tsellPrice\tmaxStock\tcoefficient\tbuyQtyMode\tbuyQtyValue\tsellQtyMode\tsellQtyValue\tdestockCoefficient\tstockBehaviorAtRestart\tattachments\tvariants\tnotes";
    static const string CATEGORIES_HEADER = "categoryKey\tcategoryName\ticon\tisVisible\tlicensesRequired\tproductKeys\tnotes";
    static const string PRESETS_HEADER = "presetKey\tproductKey\tpresetName\tisDefault\tattachments\tnotes";

    // Returns the number of exported products (0 = nothing to export, no file written)
    static int ExportLoadedCatalog(string productsPath, string categoriesPath, string presetsPath)
    {
        GetTraderXLogger().LogInfo("=== Exporting catalog to CSV ===");

        if (!FileExist(TRADERX_CSV_SOURCE_DIR))
            MakeDirectory(TRADERX_CSV_SOURCE_DIR);

        map<string, ref TraderXCsvProduct> products = new map<string, ref TraderXCsvProduct>;
        array<ref TraderXProduct> loadedProducts = TraderXProductRepository.GetProducts();
        foreach (TraderXProduct product : loadedProducts)
        {
            if (product && product.productId != "")
                products.Set(product.productId, TraderXCsvMapper.FromProduct(product.productId, product));
        }
        if (products.Count() == 0)
        {
            GetTraderXLogger().LogWarning("[TraderX] No product loaded: nothing to export to CSV");
            return 0;
        }

        map<string, ref TraderXCsvCategory> categories = new map<string, ref TraderXCsvCategory>;
        array<ref TraderXCategory> loadedCategories = TraderXCategoryRepository.GetCategories();
        foreach (TraderXCategory category : loadedCategories)
        {
            if (category && category.categoryId != "")
                categories.Set(category.categoryId, TraderXCsvMapper.FromCategory(category.categoryId, category));
        }

        array<ref TraderXCsvPreset> presets = new array<ref TraderXCsvPreset>;
        array<string> productIds = SortedProductKeys(products);
        foreach (string productId : productIds)
        {
            string presetPath = string.Format(TRADERX_PRESETS_FILE, productId);
            if (!FileExist(presetPath))
                continue;
            TraderXPresets productPresets = new TraderXPresets();
            string error;
            if (!JsonFileLoader<TraderXPresets>.LoadFile(presetPath, productPresets, error))
                continue;
            productPresets.productId = productId;
            array<ref TraderXCsvPreset> presetRows = TraderXCsvMapper.FromPresets(productPresets);
            foreach (TraderXCsvPreset row : presetRows)
            {
                presets.Insert(row);
            }
        }

        ExportProductsToCSV(products, productsPath);
        ExportCategoriesToCSV(categories, categoriesPath);
        if (presets.Count() > 0)
            ExportPresetsToCSV(presets, presetsPath);

        GetTraderXLogger().LogInfo(string.Format("[TraderX] Exported %1 products, %2 categories, %3 presets to %4", products.Count(), categories.Count(), presets.Count(), TRADERX_CSV_SOURCE_DIR));
        return products.Count();
    }

    static void ExportProductsToCSV(map<string, ref TraderXCsvProduct> products, string filePath)
    {
        FileHandle file = OpenFile(filePath, FileMode.WRITE);
        if (!file)
        {
            GetTraderXLogger().LogError("Failed to create CSV file: " + filePath);
            return;
        }

        FPrintln(file, PRODUCTS_HEADER);
        array<string> keys = SortedProductKeys(products);
        foreach (string key : keys)
        {
            TraderXCsvProduct product = products.Get(key);
            string line = product.productKey;
            line += "\t" + product.className;
            line += "\t" + product.buyPrice.ToString();
            line += "\t" + product.sellPrice.ToString();
            line += "\t" + product.maxStock.ToString();
            line += "\t" + FormatNumber(product.coefficient);
            line += "\t" + product.buyQtyMode;
            line += "\t" + FormatNumber(product.buyQtyValue);
            line += "\t" + product.sellQtyMode;
            line += "\t" + FormatNumber(product.sellQtyValue);
            line += "\t" + FormatNumber(product.destockCoefficient);
            line += "\t" + product.stockBehaviorAtRestart.ToString();
            line += "\t" + JoinList(product.attachments);
            line += "\t" + JoinList(product.variants);
            line += "\t" + product.notes;
            FPrintln(file, line);
        }
        CloseFile(file);
        GetTraderXLogger().LogInfo("Exported products to: " + filePath);
    }

    static void ExportCategoriesToCSV(map<string, ref TraderXCsvCategory> categories, string filePath)
    {
        FileHandle file = OpenFile(filePath, FileMode.WRITE);
        if (!file)
        {
            GetTraderXLogger().LogError("Failed to create CSV file: " + filePath);
            return;
        }

        FPrintln(file, CATEGORIES_HEADER);
        array<string> keys = SortedCategoryKeys(categories);
        foreach (string key : keys)
        {
            TraderXCsvCategory category = categories.Get(key);
            string visible = "0";
            if (category.isVisible)
                visible = "1";

            string line = category.categoryKey;
            line += "\t" + category.categoryName;
            line += "\t" + category.icon;
            line += "\t" + visible;
            line += "\t" + JoinList(category.licensesRequired);
            line += "\t" + JoinList(category.productKeys);
            line += "\t" + category.notes;
            FPrintln(file, line);
        }
        CloseFile(file);
        GetTraderXLogger().LogInfo("Exported categories to: " + filePath);
    }

    static void ExportPresetsToCSV(array<ref TraderXCsvPreset> presets, string filePath)
    {
        FileHandle file = OpenFile(filePath, FileMode.WRITE);
        if (!file)
        {
            GetTraderXLogger().LogError("Failed to create CSV file: " + filePath);
            return;
        }

        FPrintln(file, PRESETS_HEADER);
        foreach (TraderXCsvPreset preset : presets)
        {
            string isDefault = "0";
            if (preset.isDefault)
                isDefault = "1";

            string line = preset.presetKey;
            line += "\t" + preset.productKey;
            line += "\t" + preset.presetName;
            line += "\t" + isDefault;
            line += "\t" + JoinList(preset.attachments);
            line += "\t" + preset.notes;
            FPrintln(file, line);
        }
        CloseFile(file);
        GetTraderXLogger().LogInfo("Exported presets to: " + filePath);
    }

    // Header-only files so a new server shows the admin what to fill in
    static void WriteEmptyTemplates()
    {
        if (!FileExist(TRADERX_CSV_SOURCE_DIR))
            MakeDirectory(TRADERX_CSV_SOURCE_DIR);
        WriteLineIfMissing(TRADERX_CSV_SOURCE_DIR + "products.csv", PRODUCTS_HEADER);
        WriteLineIfMissing(TRADERX_CSV_SOURCE_DIR + "categories.csv", CATEGORIES_HEADER);
        WriteLineIfMissing(TRADERX_CSV_SOURCE_DIR + "presets.csv", PRESETS_HEADER);
    }

    private static void WriteLineIfMissing(string filePath, string line)
    {
        if (FileExist(filePath))
            return;
        FileHandle file = OpenFile(filePath, FileMode.WRITE);
        if (!file)
            return;
        FPrintln(file, line);
        CloseFile(file);
    }

    private static string JoinList(array<string> items)
    {
        string joined = "";
        if (!items)
            return joined;
        foreach (int i, string item : items)
        {
            if (i > 0)
                joined += ";";
            joined += item;
        }
        return joined;
    }

    // 1 -> "1", 0.5 -> "0.5"
    private static string FormatNumber(float value)
    {
        int whole = value;
        float wholeAsFloat = whole;
        if (wholeAsFloat == value)
            return whole.ToString();
        return value.ToString();
    }

    private static array<string> SortedProductKeys(map<string, ref TraderXCsvProduct> rows)
    {
        array<string> keys = new array<string>;
        foreach (string key, TraderXCsvProduct row : rows)
        {
            keys.Insert(key);
        }
        keys.Sort();
        return keys;
    }

    private static array<string> SortedCategoryKeys(map<string, ref TraderXCsvCategory> rows)
    {
        array<string> keys = new array<string>;
        foreach (string key, TraderXCsvCategory row : rows)
        {
            keys.Insert(key);
        }
        keys.Sort();
        return keys;
    }
}
