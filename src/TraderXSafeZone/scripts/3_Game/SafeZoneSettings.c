class TXSafeZoneSettings
{
    string version = "";
    int notificationTimer = 10; 
    string notificationTitle = "SafeZone";
    string msgOnEnteringZone = "You entered the safezone!";
    string msgOnLeavingZone = "You have left the safezone!";
    int leaveZoneCountdown = 30;
    string msgOnLeavingZoneCountdown = "You are leaving the safezone in %1 seconds";
    ref array<string> adminSteamUIDs;
    bool enableAfkDisconnect = false;
    int kickAfterDelay = 300;
    int cleanupTimer = 1200;
    ref array<string> objectsToDelete;
    ref array<ref TXSafeZoneLocation> locations;    

    void TXSafeZoneSettings()
    {
        locations = new array<ref TXSafeZoneLocation>();
        adminSteamUIDs = new array<string>();
        objectsToDelete = new array<string>();
    }

    void DefaultSettings()
    {
        version = "1.0.0";
        locations.Insert(new TXSafeZoneLocation(true, 1, "Test", Vector(0, 0, 0), 10, true, new array<string>));
        adminSteamUIDs.Insert("76561198000000000");
        objectsToDelete.Insert("Paper");
        CheckDirectories();
        SaveSettings();
    }

    void SaveSettings()
    {
        JsonFileLoader<TXSafeZoneSettings>.JsonSaveFile(Safe_Zone_Config_Filename, this);
    }

    void CheckVersion()
    {
        if(version != "1.0.0")
        {
            version = "1.0.0";
            SaveSettings();
        }
    }

    static void CheckDirectories()
    {
        if (!FileExist(Safe_Zone_Root_Server))
            MakeDirectory(Safe_Zone_Root_Server);
        if (!FileExist(Safe_Zone_Config_Dir_Server))
            MakeDirectory(Safe_Zone_Config_Dir_Server);
    }

    static TXSafeZoneSettings Load()
    {
        TXSafeZoneSettings settings = new TXSafeZoneSettings();

        CheckDirectories();

        if (FileExist(Safe_Zone_Config_Filename))
        {
            JsonFileLoader<TXSafeZoneSettings>.JsonLoadFile(Safe_Zone_Config_Filename, settings);
            settings.CheckVersion();
            return settings;
        }

        settings.DefaultSettings();
        return settings;
    }
}
