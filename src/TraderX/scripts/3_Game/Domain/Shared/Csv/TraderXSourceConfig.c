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
    bool autoMigrate;          // Auto-migrate from legacy when starting fresh
    bool allowCompilation;     // Allow CSV -> Compiled JSON compilation
    
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
    
    static TraderXSourceConfig LoadConfig()
    {
        TraderXSourceConfig config = new TraderXSourceConfig();
        
        if (FileExist(CONFIG_PATH))
        {
            JsonFileLoader<TraderXSourceConfig>.JsonLoadFile(CONFIG_PATH, config);
            GetTraderXLogger().LogInfo(string.Format("Source config loaded - Preferred format: %1, Auto-migrate: %2", config.preferredFormat, config.autoMigrate));
        }
        else
        {
            GetTraderXLogger().LogInfo("No SourceConfig.json found, using defaults (CSV, auto-migrate enabled)");
            SaveConfig(config);
        }
        
        return config;
    }
    
    static void SaveConfig(TraderXSourceConfig config)
    {
        JsonFileLoader<TraderXSourceConfig>.JsonSaveFile(CONFIG_PATH, config);
        GetTraderXLogger().LogInfo("Source config saved to: " + CONFIG_PATH);
    }
}
