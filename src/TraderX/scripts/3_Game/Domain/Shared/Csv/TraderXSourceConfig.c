// TraderX Source Format Options
enum ETraderXSourceFormat
{
    LEGACY,     // Individual JSON files in Products/ and Categories/ folders
    CSV,        // CSV files in Source/ folder (products.csv, categories.csv)
    COMPILED    // Compiled JSON files in Compiled/ folder
};

// TraderX Source Configuration - User preferences for source format
class TraderXSourceConfig
{
    string preferredFormat;     // "LEGACY", "CSV", or "COMPILED"
    bool autoMigrate;          // CSV/COMPILED: export the legacy Products/ + Categories/ files to Source/ once when Source/ is empty
    bool allowCompilation;     // CSV: compile Source/ -> Compiled/ at each boot (false = keep the current Compiled/ files)
    
    void TraderXSourceConfig()
    {
        preferredFormat = "LEGACY";  // Default to legacy for backward compatibility
        autoMigrate = false;          // Don't auto-migrate by default
        allowCompilation = true;      // Allow compilation by default
    }
    
    // Get format as enum
    ETraderXSourceFormat GetFormatEnum()
    {
        string upperFormat = preferredFormat;
        upperFormat.ToUpper();
        
        switch (upperFormat)
        {
            case "CSV": return ETraderXSourceFormat.CSV;
            case "COMPILED": return ETraderXSourceFormat.COMPILED;
            default: return ETraderXSourceFormat.LEGACY;
        }
        
        return ETraderXSourceFormat.LEGACY; // Fallback
    }
    
    // Check if this format requires source files (CSV or JSON source)
    bool RequiresSourceFiles()
    {
        ETraderXSourceFormat format = GetFormatEnum();
        return (format == ETraderXSourceFormat.CSV);
    }
    
    // Check if this format uses compiled output
    bool UsesCompiledFiles()
    {
        ETraderXSourceFormat format = GetFormatEnum();
        return (format == ETraderXSourceFormat.COMPILED);
    }
}

// Global helper to load source configuration
class TraderXSourceConfigLoader
{
    private static string CONFIG_PATH = "$profile:TraderX/TraderXConfig/SourceConfig.json";

    // freshInstall: no TraderXGeneralSettings.json existed before this boot. A new server starts on CSV;
    // an existing server without SourceConfig.json keeps the per-file JSON it was already edited with.
    static TraderXSourceConfig LoadConfig(bool freshInstall = false)
    {
        TraderXSourceConfig config = new TraderXSourceConfig();

        if (FileExist(CONFIG_PATH))
        {
            JsonFileLoader<TraderXSourceConfig>.JsonLoadFile(CONFIG_PATH, config);
            GetTraderXLogger().LogInfo(string.Format("Source config loaded - Preferred format: %1, Auto-migrate: %2", config.preferredFormat, config.autoMigrate));
            return config;
        }

        if (freshInstall)
        {
            config.preferredFormat = "CSV";
            config.autoMigrate = true;
            GetTraderXLogger().LogInfo("No SourceConfig.json on a new install: using CSV (Source/*.csv)");
        }
        else
        {
            GetTraderXLogger().LogInfo("No SourceConfig.json on an existing install: keeping LEGACY (Products/*.json, Categories/*.json)");
        }
        SaveConfig(config);
        return config;
    }

    static void SaveConfig(TraderXSourceConfig config)
    {
        JsonFileLoader<TraderXSourceConfig>.JsonSaveFile(CONFIG_PATH, config);
        GetTraderXLogger().LogInfo("Source config saved to: " + CONFIG_PATH);
    }
}
