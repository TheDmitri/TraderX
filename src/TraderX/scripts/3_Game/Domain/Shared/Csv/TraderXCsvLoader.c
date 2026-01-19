// TraderX CSV Loader - Parses CSV files into data structures
class TraderXCsvLoader
{
    // Parse a CSV file into array of string arrays (rows and columns)
    static array<ref array<string>> ParseCsvFile(string filePath, string delimiter = "\t")
    {
        array<ref array<string>> rows = new array<ref array<string>>;
        
        if (!FileExist(filePath))
        {
            GetTraderXLogger().LogError("[TraderXCsvLoader] File not found: " + filePath);
            return rows;
        }
        
        FileHandle file = OpenFile(filePath, FileMode.READ);
        if (file == 0)
        {
            GetTraderXLogger().LogError("[TraderXCsvLoader] Failed to open file: " + filePath);
            return rows;
        }
        
        string line;
        int lineNumber = 0;
        
        while (FGets(file, line) > 0)
        {
            lineNumber++;
            
            // Skip empty lines
            line.Trim();
            if (line.Length() == 0)
                continue;
            
            // Parse line into columns
            array<string> columns = new array<string>;
            ParseCsvLine(line, delimiter, columns);
            
            rows.Insert(columns);
        }
        
        CloseFile(file);
        
        GetTraderXLogger().LogDebug(string.Format("[TraderXCsvLoader] Parsed %1 lines from %2", lineNumber, filePath));
        
        return rows;
    }
    
    // Parse a single CSV line respecting quoted fields
    private static void ParseCsvLine(string line, string delimiter, out array<string> columns)
    {
        bool inQuotes = false;
        string currentField = "";
        int len = line.Length();
        
        for (int i = 0; i < len; i++)
        {
            string ch = line.Get(i);
            
            // Handle quotes
            if (ch == "\"")
            {
                inQuotes = !inQuotes;
                continue;
            }
            
            // Handle delimiter
            if (ch == delimiter && !inQuotes)
            {
                currentField.Trim();
                columns.Insert(currentField);
                currentField = "";
                continue;
            }
            
            // Append character to current field
            currentField += ch;
        }
        
        // Add last field
        currentField.Trim();
        columns.Insert(currentField);
    }
    
    // Load products from CSV file
    static array<ref TraderXCsvProduct> LoadProductsCsv(string filePath)
    {
        array<ref TraderXCsvProduct> products = new array<ref TraderXCsvProduct>;
        
        array<ref array<string>> rows = ParseCsvFile(filePath);
        
        if (rows.Count() == 0)
        {
            GetTraderXLogger().LogWarning("[TraderXCsvLoader] No data in products CSV: " + filePath);
            return products;
        }
        
        // First row is header
        array<string> headers = rows.Get(0);
        map<string, int> columnMap = new map<string, int>();
        
        // Map column names to indices
        for (int i = 0; i < headers.Count(); i++)
        {
            string header = headers.Get(i);
            header.ToLower();
            columnMap.Set(header, i);
        }
        
        // Parse data rows
        for (int rowIdx = 1; rowIdx < rows.Count(); rowIdx++)
        {
            array<string> row = rows.Get(rowIdx);
            
            // Skip empty rows
            if (row.Count() == 0 || row.Get(0).Length() == 0)
                continue;
            
            TraderXCsvProduct product = new TraderXCsvProduct();
            
            // Parse each column
            product.productKey = GetCsvValue(row, columnMap, "productkey");
            product.className = GetCsvValue(row, columnMap, "classname");
            product.buyPrice = GetCsvValueInt(row, columnMap, "buyprice");
            product.sellPrice = GetCsvValueInt(row, columnMap, "sellprice");
            product.maxStock = GetCsvValueInt(row, columnMap, "maxstock");
            product.buyQtyMode = GetCsvValue(row, columnMap, "buyqtymode");
            product.buyQtyValue = GetCsvValueFloat(row, columnMap, "buyqtyvalue");
            product.sellQtyMode = GetCsvValue(row, columnMap, "sellqtymode");
            product.sellQtyValue = GetCsvValueFloat(row, columnMap, "sellqtyvalue");
            product.destockCoefficient = GetCsvValueFloat(row, columnMap, "destockcoefficient");
            product.stockBehaviorAtRestart = GetCsvValueInt(row, columnMap, "stockbehavioratrestart");
            
            // Parse semicolon-separated lists
            string attachmentsStr = GetCsvValue(row, columnMap, "attachments");
            if (attachmentsStr.Length() > 0)
            {
                product.attachments = new ref array<string>;
                ParseSemicolonList(attachmentsStr, product.attachments);
            }
            
            string variantsStr = GetCsvValue(row, columnMap, "variants");
            if (variantsStr.Length() > 0)
            {
                product.variants = new ref array<string>;
                ParseSemicolonList(variantsStr, product.variants);
            }
            
            product.notes = GetCsvValue(row, columnMap, "notes");
            
            products.Insert(product);
        }
        
        GetTraderXLogger().LogInfo(string.Format("[TraderXCsvLoader] Loaded %1 products from %2", products.Count(), filePath));
        
        return products;
    }
    
    // Load categories from CSV file
    static array<ref TraderXCsvCategory> LoadCategoriesCsv(string filePath)
    {
        array<ref TraderXCsvCategory> categories = new array<ref TraderXCsvCategory>;
        
        array<ref array<string>> rows = ParseCsvFile(filePath);
        
        if (rows.Count() == 0)
        {
            GetTraderXLogger().LogWarning("[TraderXCsvLoader] No data in categories CSV: " + filePath);
            return categories;
        }
        
        // First row is header
        array<string> headers = rows.Get(0);
        map<string, int> columnMap = new map<string, int>();
        
        for (int i = 0; i < headers.Count(); i++)
        {
            string header = headers.Get(i);
            header.ToLower();
            columnMap.Set(header, i);
        }
        
        // Parse data rows
        for (int rowIdx = 1; rowIdx < rows.Count(); rowIdx++)
        {
            array<string> row = rows.Get(rowIdx);
            
            if (row.Count() == 0 || row.Get(0).Length() == 0)
                continue;
            
            TraderXCsvCategory category = new TraderXCsvCategory();
            
            category.categoryKey = GetCsvValue(row, columnMap, "categorykey");
            category.categoryName = GetCsvValue(row, columnMap, "categoryname");
            category.icon = GetCsvValue(row, columnMap, "icon");
            category.isVisible = GetCsvValueBool(row, columnMap, "isvisible");
            
            // Parse semicolon-separated lists
            string licensesStr = GetCsvValue(row, columnMap, "licensesrequired");
            if (licensesStr.Length() > 0)
            {
                category.licensesRequired = new ref array<string>;
                ParseSemicolonList(licensesStr, category.licensesRequired);
            }
            
            string productsStr = GetCsvValue(row, columnMap, "productkeys");
            if (productsStr.Length() > 0)
            {
                category.productKeys = new ref array<string>;
                ParseSemicolonList(productsStr, category.productKeys);
            }
            
            category.notes = GetCsvValue(row, columnMap, "notes");
            
            categories.Insert(category);
        }
        
        GetTraderXLogger().LogInfo(string.Format("[TraderXCsvLoader] Loaded %1 categories from %2", categories.Count(), filePath));
        
        return categories;
    }
    
    // Load presets from CSV file
    static array<ref TraderXCsvPreset> LoadPresetsCsv(string filePath)
    {
        array<ref TraderXCsvPreset> presets = new array<ref TraderXCsvPreset>;
        
        array<ref array<string>> rows = ParseCsvFile(filePath);
        
        if (rows.Count() == 0)
        {
            GetTraderXLogger().LogWarning("[TraderXCsvLoader] No data in presets CSV: " + filePath);
            return presets;
        }
        
        // First row is header
        array<string> headers = rows.Get(0);
        map<string, int> columnMap = new map<string, int>();
        
        for (int i = 0; i < headers.Count(); i++)
        {
            string header = headers.Get(i);
            header.ToLower();
            columnMap.Set(header, i);
        }
        
        // Parse data rows
        for (int rowIdx = 1; rowIdx < rows.Count(); rowIdx++)
        {
            array<string> row = rows.Get(rowIdx);
            
            if (row.Count() == 0 || row.Get(0).Length() == 0)
                continue;
            
            TraderXCsvPreset preset = new TraderXCsvPreset();
            
            preset.presetKey = GetCsvValue(row, columnMap, "presetkey");
            preset.productKey = GetCsvValue(row, columnMap, "productkey");
            preset.presetName = GetCsvValue(row, columnMap, "presetname");
            
            // Parse semicolon-separated attachment list
            string attachmentsStr = GetCsvValue(row, columnMap, "attachments");
            if (attachmentsStr.Length() > 0)
            {
                preset.attachments = new ref array<string>;
                ParseSemicolonList(attachmentsStr, preset.attachments);
            }
            
            preset.notes = GetCsvValue(row, columnMap, "notes");
            
            presets.Insert(preset);
        }
        
        GetTraderXLogger().LogInfo(string.Format("[TraderXCsvLoader] Loaded %1 presets from %2", presets.Count(), filePath));
        
        return presets;
    }
    
    // Helper: Get string value from CSV row
    private static string GetCsvValue(array<string> row, map<string, int> columnMap, string columnName)
    {
        int idx;
        if (columnMap.Find(columnName, idx))
        {
            if (idx < row.Count())
            {
                string val = row.Get(idx);
                val.Trim();
                return val;
            }
        }
        return "";
    }
    
    // Helper: Get int value from CSV row
    private static int GetCsvValueInt(array<string> row, map<string, int> columnMap, string columnName)
    {
        string val = GetCsvValue(row, columnMap, columnName);
        if (val.Length() == 0)
            return 0;
        return val.ToInt();
    }
    
    // Helper: Get float value from CSV row
    private static float GetCsvValueFloat(array<string> row, map<string, int> columnMap, string columnName)
    {
        string val = GetCsvValue(row, columnMap, columnName);
        if (val.Length() == 0)
            return 0.0;
        return val.ToFloat();
    }
    
    // Helper: Get bool value from CSV row
    private static bool GetCsvValueBool(array<string> row, map<string, int> columnMap, string columnName)
    {
        string val = GetCsvValue(row, columnMap, columnName);
        val.ToLower();
        return (val == "1" || val == "true" || val == "yes");
    }
    
    // Helper: Parse semicolon-separated list
    private static void ParseSemicolonList(string input, out array<string> output)
    {
        TStringArray parts = new TStringArray;
        input.Split(";", parts);
        
        foreach (string part : parts)
        {
            part.Trim();
            if (part.Length() > 0)
            {
                output.Insert(part);
            }
        }
    }
}
