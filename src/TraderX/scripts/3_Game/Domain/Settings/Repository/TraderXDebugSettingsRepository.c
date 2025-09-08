/**
 * TraderXDebugSettingsRepository - Handles persistence of debug settings
 * Manages loading, saving, and validation of debug configuration
 */
class TraderXDebugSettingsRepository
{
    /**
     * Save debug settings to file
     */
    static void Save(TraderXDebugSettings settings)
    {
        settings.ValidateSettings();
        TraderXJsonLoader<TraderXDebugSettings>.SaveToFile(TRADERX_DEBUG_SETTINGS_FILE, settings);
    }
    
    /**
     * Load debug settings from file, create default if not exists
     */
    static ref TraderXDebugSettings Load()
    {
        MakeDirectoriesIfNotExist();
        
        TraderXDebugSettings settings = new TraderXDebugSettings();
        
        if (FileExist(TRADERX_DEBUG_SETTINGS_FILE))
        {
            TraderXJsonLoader<TraderXDebugSettings>.LoadFromFile(TRADERX_DEBUG_SETTINGS_FILE, settings);
            
            if (!settings)
            {
                GetTraderXLogger().LogWarning("[DEBUG SETTINGS] Failed to load debug settings, creating defaults");
                settings = CreateDefaultSettings();
            }
            else
            {
                CheckVersion(settings);
            }
        }
        else
        {
            GetTraderXLogger().LogDebug("[DEBUG SETTINGS] Debug settings file not found, creating defaults");
            settings = CreateDefaultSettings();
        }

        return settings;
    }
    
    /**
     * Check version and update if necessary
     */
    static void CheckVersion(TraderXDebugSettings settings)
    {
        bool updated = false;
        
        if (settings.version != TRADERX_CURRENT_VERSION)
        {
            settings.version = TRADERX_CURRENT_VERSION;
            updated = true;
        }
        
        if (updated)
        {
            GetTraderXLogger().LogDebug("[DEBUG SETTINGS] Updated debug settings to new version");
            Save(settings);
        }
    }
    
    /**
     * Create default debug settings
     */
    static ref TraderXDebugSettings CreateDefaultSettings()
    {
        TraderXDebugSettings settings = new TraderXDebugSettings();
        settings.DefaultTraderXDebugSettings();
        Save(settings);
        
        GetTraderXLogger().LogDebug("[DEBUG SETTINGS] Created default debug settings");
        return settings;
    }
    
    /**
     * Create directories if they don't exist
     */
    static void MakeDirectoriesIfNotExist()
    {
        if (!FileExist(TRADERX_CONFIG_DIR_SERVER))
        {
            MakeDirectory(TRADERX_CONFIG_DIR_SERVER);
        }
        
        if (!FileExist(TRADERX_DEBUG_TEST_CASES_DIR))
        {
            MakeDirectory(TRADERX_DEBUG_TEST_CASES_DIR);
        }
    }
    
    /**
     * Reset debug settings to defaults
     */
    static void ResetToDefaults()
    {
        TraderXDebugSettings settings = new TraderXDebugSettings();
        settings.DefaultTraderXDebugSettings();
        Save(settings);
        
        GetTraderXLogger().LogDebug("[DEBUG SETTINGS] Reset debug settings to defaults");
    }
    
    /**
     * Enable debug mode and save
     */
    static void EnableDebugMode(bool enable = true)
    {
        TraderXDebugSettings settings = Load();
        settings.enableDebugMode = enable;
        Save(settings);
        
        if(enable)
        {
            GetTraderXLogger().LogDebug("[DEBUG SETTINGS] Debug mode ENABLED");
        }
        else
        {
            GetTraderXLogger().LogDebug("[DEBUG SETTINGS] Debug mode DISABLED");
        }
    }
    
    /**
     * Enable test suite execution and save
     */
    static void EnableTestSuiteExecution(bool enable = true)
    {
        TraderXDebugSettings settings = Load();
        settings.runTestsOnStartup = enable;
        Save(settings);
        
        if(enable)
        {
            GetTraderXLogger().LogDebug("[DEBUG SETTINGS] Test suite execution ENABLED");
        }
        else
        {
            GetTraderXLogger().LogDebug("[DEBUG SETTINGS] Test suite execution DISABLED");
        }
    }
}
