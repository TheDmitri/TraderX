// Server-side settings, persisted as JSON under $profile. Synced to clients on connect.
// passwordLength = 8 is the project requirement (TheLab used 4).

class TXCLSettings
{
    string version = TXCL_VERSION;
    string carLockClassName = "CarLock";
    int    passwordLength = 8;
    int    initCooldownTime = 10;

    ref array<string> admins;
    ref TXCLLockpickingSettings lockpickingSettings;

    void TXCLSettings()
    {
        admins = new array<string>();
        lockpickingSettings = new TXCLLockpickingSettings();
    }

    static void MakeDirectoriesIfNotExist()
    {
        if (!FileExist(TXCL_PROFILE_ROOT))
            MakeDirectory(TXCL_PROFILE_ROOT);
        if (!FileExist(TXCL_CONFIG_ROOT))
            MakeDirectory(TXCL_CONFIG_ROOT);
    }

    void ApplyDefaults()
    {
        Save();
    }

    void Save()
    {
        JsonFileLoader<TXCLSettings>.JsonSaveFile(TXCL_CONFIG_FILENAME, this);
    }

    static ref TXCLSettings Load()
    {
        TXCLSettings settings = new TXCLSettings();

        MakeDirectoriesIfNotExist();

        if (FileExist(TXCL_CONFIG_FILENAME))
        {
            JsonFileLoader<TXCLSettings>.JsonLoadFile(TXCL_CONFIG_FILENAME, settings);
            return settings;
        }

        settings.ApplyDefaults();
        return settings;
    }
}
