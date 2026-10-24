// TraderX CSV Loader - Parses the Source/*.csv files into CSV models.
//
// Format: first line = header (column names, any order, case-insensitive), one row per line.
// Delimiter: TAB, or comma / semicolon when the header has no TAB. Fields may be quoted ("a;b"; "" = a quote),
// as spreadsheets do for a cell holding the delimiter. Lists (attachments, productKeys...) are separated by ';'.
// Booleans: 1/true/yes or 0/false/no. Decimal comma accepted (0,5).
// Blank cells take the defaults of TraderXCsvModels.c (prices -1, maxStock -1, coefficient 1, modes FULL).
class TraderXCsvLoader
{
    // File names probed when FindFile cannot list Source/ (DayZ 1.30 with an external -profiles path)
    static TStringArray KnownSourceFiles()
    {
        TStringArray files = {"products.csv", "categories.csv", "presets.csv", "products_migrated.csv", "categories_migrated.csv", "presets_migrated.csv", "00_imported_products.csv", "00_imported_categories.csv", "00_imported_presets.csv"};
        return files;
    }

    // Sorted names of the files matching *.<ext> in dir
    static array<string> ListFiles(string dir, string ext)
    {
        array<string> files = new array<string>;
        string filename;
        FileAttr attr;
        FindFileHandle handle = TraderXFileSystem.Find(dir + "*." + ext, filename, attr, FindFileFlags.ALL);
        if (handle != 0)
        {
            files.Insert(filename);
            while (FindNextFile(handle, filename, attr))
            {
                files.Insert(filename);
            }
            CloseFindFile(handle);
        }
        else if (ext == "csv")
        {
            foreach (string known : KnownSourceFiles())
            {
                if (FileExist(dir + known))
                    files.Insert(known);
            }
        }
        files.Sort();
        return files;
    }

    // Parse a CSV file into rows of fields; lineNumbers[i] is the file line of rows[i]
    static array<ref array<string>> ParseCsvFile(string filePath, out array<int> lineNumbers)
    {
        array<ref array<string>> rows = new array<ref array<string>>;
        lineNumbers = new array<int>;

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
        string delimiter = "\t";

        while (FGets(file, line) >= 0)
        {
            lineNumber++;
            line.TrimInPlace();
            if (line.Length() == 0)
                continue;

            // The header decides the delimiter: TAB, else comma, else semicolon (French/German Excel "CSV")
            if (rows.Count() == 0 && line.IndexOf("\t") == -1)
            {
                if (line.IndexOf(",") != -1)
                    delimiter = ",";
                else if (line.IndexOf(";") != -1)
                    delimiter = ";";
            }

            array<string> columns = new array<string>;
            ParseCsvLine(line, delimiter, columns);
            rows.Insert(columns);
            lineNumbers.Insert(lineNumber);
        }

        CloseFile(file);
        GetTraderXLogger().LogDebug(string.Format("[TraderXCsvLoader] Parsed %1 lines from %2", lineNumber, filePath));
        return rows;
    }

    // Parse a single CSV line respecting quoted fields ("" inside quotes = literal quote)
    private static void ParseCsvLine(string line, string delimiter, array<string> columns)
    {
        bool inQuotes = false;
        string currentField = "";
        int len = line.Length();

        for (int i = 0; i < len; i++)
        {
            string ch = line.Get(i);

            if (ch == "\"")
            {
                if (inQuotes && i + 1 < len && line.Get(i + 1) == "\"")
                {
                    currentField += "\"";
                    i++;
                    continue;
                }
                inQuotes = !inQuotes;
                continue;
            }

            if (ch == delimiter && !inQuotes)
            {
                currentField.TrimInPlace();
                columns.Insert(currentField);
                currentField = "";
                continue;
            }

            currentField += ch;
        }

        currentField.TrimInPlace();
        columns.Insert(currentField);
    }

    // Header name -> column index. Drops anything before the first letter (UTF-8 BOM written by Excel).
    private static map<string, int> MapHeader(array<string> headers)
    {
        map<string, int> columnMap = new map<string, int>();
        for (int i = 0; i < headers.Count(); i++)
        {
            string header = headers.Get(i);
            header.ToLower();
            while (header.Length() > 0 && (header.Get(0) < "a" || header.Get(0) > "z"))
            {
                header = header.Substring(1, header.Length() - 1);
            }
            columnMap.Set(header, i);
        }
        return columnMap;
    }

    private static string FileNameOf(string filePath)
    {
        int slash = filePath.LastIndexOf("/");
        int backslash = filePath.LastIndexOf("\\");
        if (backslash > slash)
            slash = backslash;
        if (slash < 0)
            return filePath;
        return filePath.Substring(slash + 1, filePath.Length() - slash - 1);
    }

    static array<ref TraderXCsvProduct> LoadProductsCsv(string filePath)
    {
        array<ref TraderXCsvProduct> products = new array<ref TraderXCsvProduct>;
        array<int> lines;
        array<ref array<string>> rows = ParseCsvFile(filePath, lines);
        if (rows.Count() == 0)
        {
            GetTraderXLogger().LogWarning("[TraderXCsvLoader] No data in products CSV: " + filePath);
            return products;
        }

        map<string, int> columnMap = MapHeader(rows.Get(0));
        string source = FileNameOf(filePath);

        for (int rowIdx = 1; rowIdx < rows.Count(); rowIdx++)
        {
            array<string> row = rows.Get(rowIdx);
            if (row.Count() == 0 || row.Get(0).Length() == 0)
                continue;

            TraderXCsvProduct product = new TraderXCsvProduct();
            product.sourceFile = source;
            product.sourceRow = lines.Get(rowIdx);
            product.productKey = GetCsvValue(row, columnMap, "productkey");
            product.className = GetCsvValue(row, columnMap, "classname");
            product.buyPrice = GetCsvValueInt(row, columnMap, "buyprice", product.buyPrice);
            product.sellPrice = GetCsvValueInt(row, columnMap, "sellprice", product.sellPrice);
            product.maxStock = GetCsvValueInt(row, columnMap, "maxstock", product.maxStock);
            product.coefficient = GetCsvValueFloat(row, columnMap, "coefficient", product.coefficient);
            product.buyQtyMode = GetCsvValueUpper(row, columnMap, "buyqtymode", product.buyQtyMode);
            product.buyQtyValue = GetCsvValueFloat(row, columnMap, "buyqtyvalue", 0.0);
            product.sellQtyMode = GetCsvValueUpper(row, columnMap, "sellqtymode", product.sellQtyMode);
            product.sellQtyValue = GetCsvValueFloat(row, columnMap, "sellqtyvalue", 0.0);
            product.destockCoefficient = GetCsvValueFloat(row, columnMap, "destockcoefficient", 0.0);
            product.stockBehaviorAtRestart = GetCsvValueInt(row, columnMap, "stockbehavioratrestart", 0);
            product.attachments = GetCsvList(row, columnMap, "attachments");
            product.variants = GetCsvList(row, columnMap, "variants");
            product.notes = GetCsvValue(row, columnMap, "notes");
            products.Insert(product);
        }

        GetTraderXLogger().LogInfo(string.Format("[TraderXCsvLoader] Loaded %1 products from %2", products.Count(), filePath));
        return products;
    }

    static array<ref TraderXCsvCategory> LoadCategoriesCsv(string filePath)
    {
        array<ref TraderXCsvCategory> categories = new array<ref TraderXCsvCategory>;
        array<int> lines;
        array<ref array<string>> rows = ParseCsvFile(filePath, lines);
        if (rows.Count() == 0)
        {
            GetTraderXLogger().LogWarning("[TraderXCsvLoader] No data in categories CSV: " + filePath);
            return categories;
        }

        map<string, int> columnMap = MapHeader(rows.Get(0));
        string source = FileNameOf(filePath);

        for (int rowIdx = 1; rowIdx < rows.Count(); rowIdx++)
        {
            array<string> row = rows.Get(rowIdx);
            if (row.Count() == 0 || row.Get(0).Length() == 0)
                continue;

            TraderXCsvCategory category = new TraderXCsvCategory();
            category.sourceFile = source;
            category.sourceRow = lines.Get(rowIdx);
            category.categoryKey = GetCsvValue(row, columnMap, "categorykey");
            category.categoryName = GetCsvValue(row, columnMap, "categoryname");
            category.icon = GetCsvValue(row, columnMap, "icon");
            category.isVisible = GetCsvValueBool(row, columnMap, "isvisible", true);
            category.licensesRequired = GetCsvList(row, columnMap, "licensesrequired");
            category.productKeys = GetCsvList(row, columnMap, "productkeys");
            category.notes = GetCsvValue(row, columnMap, "notes");
            categories.Insert(category);
        }

        GetTraderXLogger().LogInfo(string.Format("[TraderXCsvLoader] Loaded %1 categories from %2", categories.Count(), filePath));
        return categories;
    }

    static array<ref TraderXCsvPreset> LoadPresetsCsv(string filePath)
    {
        array<ref TraderXCsvPreset> presets = new array<ref TraderXCsvPreset>;
        array<int> lines;
        array<ref array<string>> rows = ParseCsvFile(filePath, lines);
        if (rows.Count() == 0)
        {
            GetTraderXLogger().LogWarning("[TraderXCsvLoader] No data in presets CSV: " + filePath);
            return presets;
        }

        map<string, int> columnMap = MapHeader(rows.Get(0));
        string source = FileNameOf(filePath);

        for (int rowIdx = 1; rowIdx < rows.Count(); rowIdx++)
        {
            array<string> row = rows.Get(rowIdx);
            if (row.Count() == 0 || row.Get(0).Length() == 0)
                continue;

            TraderXCsvPreset preset = new TraderXCsvPreset();
            preset.sourceFile = source;
            preset.sourceRow = lines.Get(rowIdx);
            preset.presetKey = GetCsvValue(row, columnMap, "presetkey");
            preset.productKey = GetCsvValue(row, columnMap, "productkey");
            preset.presetName = GetCsvValue(row, columnMap, "presetname");
            preset.isDefault = GetCsvValueBool(row, columnMap, "isdefault", false);
            preset.attachments = GetCsvList(row, columnMap, "attachments");
            preset.notes = GetCsvValue(row, columnMap, "notes");
            presets.Insert(preset);
        }

        GetTraderXLogger().LogInfo(string.Format("[TraderXCsvLoader] Loaded %1 presets from %2", presets.Count(), filePath));
        return presets;
    }

    private static string GetCsvValue(array<string> row, map<string, int> columnMap, string columnName)
    {
        int idx;
        if (columnMap.Find(columnName, idx) && idx < row.Count())
        {
            string val = row.Get(idx);
            val.TrimInPlace();
            return val;
        }
        return "";
    }

    private static string GetCsvValueUpper(array<string> row, map<string, int> columnMap, string columnName, string defaultValue)
    {
        string val = GetCsvValue(row, columnMap, columnName);
        if (val.Length() == 0)
            return defaultValue;
        val.ToUpper();
        return val;
    }

    private static int GetCsvValueInt(array<string> row, map<string, int> columnMap, string columnName, int defaultValue)
    {
        string val = GetCsvValue(row, columnMap, columnName);
        if (val.Length() == 0)
            return defaultValue;
        return val.ToInt();
    }

    // Accepts a decimal comma (0,5) as written by French/German spreadsheets
    private static float GetCsvValueFloat(array<string> row, map<string, int> columnMap, string columnName, float defaultValue)
    {
        string val = GetCsvValue(row, columnMap, columnName);
        if (val.Length() == 0)
            return defaultValue;
        val.Replace(",", ".");
        return val.ToFloat();
    }

    private static bool GetCsvValueBool(array<string> row, map<string, int> columnMap, string columnName, bool defaultValue)
    {
        string val = GetCsvValue(row, columnMap, columnName);
        if (val.Length() == 0)
            return defaultValue;
        val.ToLower();
        return (val == "1" || val == "true" || val == "yes");
    }

    private static array<string> GetCsvList(array<string> row, map<string, int> columnMap, string columnName)
    {
        array<string> output = new array<string>;
        string input = GetCsvValue(row, columnMap, columnName);
        if (input.Length() == 0)
            return output;

        TStringArray parts = new TStringArray;
        input.Split(";", parts);
        foreach (string part : parts)
        {
            part.TrimInPlace();
            if (part.Length() > 0)
                output.Insert(part);
        }
        return output;
    }
}
