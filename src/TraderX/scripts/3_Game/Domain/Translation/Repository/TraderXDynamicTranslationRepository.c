class TraderXDynamicTranslationRepository
{
    private static ref TraderXDynamicTranslationSettings s_Settings;
    
    static TraderXDynamicTranslationSettings GetSettings()
    {
        if (!s_Settings) {
            s_Settings = Load();
        }
        return s_Settings;
    }
    
    static void Save(TraderXDynamicTranslationSettings settings)
    {
        JsonFileLoader<TraderXDynamicTranslationSettings>.JsonSaveFile(TRADERX_DYNAMIC_TRANSLATION_CONFIG_FILE, settings);
        s_Settings = settings;
    }
    
    static TraderXDynamicTranslationSettings Load()
    {
        TraderXDynamicTranslationSettings settings = new TraderXDynamicTranslationSettings();
        
        MakeDirectoriesIfNotExist();
        
        if (FileExist(TRADERX_DYNAMIC_TRANSLATION_CONFIG_FILE)) {
            JsonFileLoader<TraderXDynamicTranslationSettings>.JsonLoadFile(TRADERX_DYNAMIC_TRANSLATION_CONFIG_FILE, settings);
            CheckVersion(settings);
        } else {
            // Create default config file
            Save(settings);
        }
        
        return settings;
    }
    
    static void CheckVersion(TraderXDynamicTranslationSettings settings)
    {
        if (settings.version != TRADERX_CURRENT_VERSION) {
            settings.version = TRADERX_CURRENT_VERSION;
            Save(settings);
        }
    }
    
    static void MakeDirectoriesIfNotExist()
    {
        if (!FileExist(TRADERX_CONFIG_ROOT_SERVER))
            MakeDirectory(TRADERX_CONFIG_ROOT_SERVER);
            
        if (!FileExist(TRADERX_CONFIG_DIR_SERVER))
            MakeDirectory(TRADERX_CONFIG_DIR_SERVER);
    }
}
