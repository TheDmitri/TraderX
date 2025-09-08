class TraderXLoggingSettings 
{
    int logLevel = 0;
    int refreshRateInSeconds = 60;

    void MakeDirectoryIfNotExists()
    {
        if(!FileExist(TRADERX_CONFIG_ROOT_SERVER))
            MakeDirectory(TRADERX_CONFIG_ROOT_SERVER);

        if(!FileExist(TRADERX_LOG_FOLDER))
            MakeDirectory(TRADERX_LOG_FOLDER);

        if(!FileExist(TRADERX_LOGGER_CONFIG_DIR))
            MakeDirectory(TRADERX_LOGGER_CONFIG_DIR);
        
        if(!FileExist(TRADERX_LOGGER_LOG_DIR))
            MakeDirectory(TRADERX_LOGGER_LOG_DIR);
    }

    void Save()
    {
        JsonFileLoader<TraderXLoggingSettings>.JsonSaveFile(TRADERX_LOGGER_CONFIG_FILE, this);
    }

    static ref TraderXLoggingSettings Load()
    {
        TraderXLoggingSettings settings = new TraderXLoggingSettings();

        settings.MakeDirectoryIfNotExists();

        if(FileExist(TRADERX_LOGGER_CONFIG_FILE))
        {
            JsonFileLoader<TraderXLoggingSettings>.JsonLoadFile(TRADERX_LOGGER_CONFIG_FILE, settings);
            return settings;
        }

        settings.Save();
        return settings;
    }

}