// TraderX Source Configuration - User preferences for source format
class TraderXSourceConfig
{
    string preferredFormat;
    bool autoMigrate;
    
    void TraderXSourceConfig()
    {
        preferredFormat = "CSV";
        autoMigrate = true;
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
