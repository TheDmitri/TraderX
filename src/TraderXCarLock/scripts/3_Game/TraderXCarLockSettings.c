class TXCarLockSettings
{
    string version = TX_CARLOCK_VERSION;
    string carLockClassName = "CarLock";

    ref array<string> admins;
    ref TXLockpickingSettings lockpickingSettings;

    void TXCarLockSettings()
    {
        admins = new array<string>();
        lockpickingSettings = new TXLockpickingSettings();
    }

    static void MakeDirectoriesIfNotExist()
    {
        if (!FileExist(TX_CARLOCK_CONFIG_ROOT))
        {
            MakeDirectory(TX_CARLOCK_CONFIG_ROOT);
        }
    }

    void TXCarLockDefaultSettings()
    {
        admins.Insert("76561198047475642");
        Save();
    }

    void Save()
    {
        JsonFileLoader<TXCarLockSettings>.JsonSaveFile(TX_CARLOCK_CONFIG_FILENAME, this);
    }

    static ref TXCarLockSettings Load()
    {
        TXCarLockSettings settings = new TXCarLockSettings();

        MakeDirectoriesIfNotExist();

        if (FileExist(TX_CARLOCK_CONFIG_FILENAME))
        {
            JsonFileLoader<TXCarLockSettings>.JsonLoadFile(TX_CARLOCK_CONFIG_FILENAME, settings);
            return settings;
        }

        settings.TXCarLockDefaultSettings();
        return settings;
    }
}
