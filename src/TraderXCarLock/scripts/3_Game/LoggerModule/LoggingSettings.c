class TXCarLockLoggingSettings
{
    int logLevel = 0;
    int refreshRateInSeconds = 60;

    void MakeDirectoryIfNotExists()
    {
        if (!FileExist(TX_CARLOCK_LOGGER_ROOT))
            MakeDirectory(TX_CARLOCK_LOGGER_ROOT);

        if (!FileExist(TX_CARLOCK_LOGGER_LOG_DIR))
            MakeDirectory(TX_CARLOCK_LOGGER_LOG_DIR);

        if (!FileExist(TX_CARLOCK_LOGGER_CONFIG_DIR))
            MakeDirectory(TX_CARLOCK_LOGGER_CONFIG_DIR);
    }

    void Save()
    {
        JsonFileLoader<TXCarLockLoggingSettings>.JsonSaveFile(TX_CARLOCK_LOGGER_CONFIG_FILE, this);
    }

    static ref TXCarLockLoggingSettings Load()
    {
        TXCarLockLoggingSettings settings = new TXCarLockLoggingSettings();

        settings.MakeDirectoryIfNotExists();

        if (FileExist(TX_CARLOCK_LOGGER_CONFIG_FILE))
        {
            JsonFileLoader<TXCarLockLoggingSettings>.JsonLoadFile(TX_CARLOCK_LOGGER_CONFIG_FILE, settings);
            return settings;
        }

        settings.Save();
        return settings;
    }
}
