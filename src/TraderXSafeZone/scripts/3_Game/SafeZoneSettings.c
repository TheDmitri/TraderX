// TraderPlus V1 TraderPlusSafeZoneConfig.json (only the fields TraderX supports are read; the
// TraderPlus stash/hideout feature is intentionally not migrated).
class TXSZTraderPlusZone
{
    string SafeZoneStatut;
    float  X;
    float  Y;
    float  Radius;
    int    Countdown;
}

class TXSZTraderPlusConfig
{
    bool   EnableAfkDisconnect;
    int    KickAfterDelay;
    string MsgEnterZone;
    string MsgExitZone;
    string MsgOnLeavingZone;
    int    CleanUpTimer;
    ref array<ref TXSZTraderPlusZone> SafeAreaLocation;
    ref array<string> WhitelistEntities;
    ref array<string> ObjectsToDelete;
    ref array<string> SZSteamUIDs;
}

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

    // One-time relocation: pre-relocation builds stored settings inside core's
    // TraderXConfig/ dir under a double-prefixed name (TraderXTXSafeZoneSettings.json).
    // Move it into the mod's own folder so core config migration/backup never touches it.
    static void MigrateLegacyConfig()
    {
        if (FileExist(Safe_Zone_Config_Filename))
            return;
        if (!FileExist(Safe_Zone_Legacy_Config_Filename))
            return;

        TXSafeZoneSettings legacy = new TXSafeZoneSettings();
        JsonFileLoader<TXSafeZoneSettings>.JsonLoadFile(Safe_Zone_Legacy_Config_Filename, legacy);
        JsonFileLoader<TXSafeZoneSettings>.JsonSaveFile(Safe_Zone_Config_Filename, legacy);
        DeleteFile(Safe_Zone_Legacy_Config_Filename);
    }

    static TXSafeZoneSettings Load()
    {
        TXSafeZoneSettings settings = new TXSafeZoneSettings();

        CheckDirectories();
        MigrateLegacyConfig();

        if (FileExist(Safe_Zone_Config_Filename))
        {
            JsonFileLoader<TXSafeZoneSettings>.JsonLoadFile(Safe_Zone_Config_Filename, settings);
            settings.CheckVersion();
            return settings;
        }

        if (settings.ImportTraderPlus())
            return settings;

        settings.DefaultSettings();
        return settings;
    }

    static const string TRADERPLUS_SAFEZONE_CONFIG = "$profile:TraderPlus/TraderPlusConfig/TraderPlusSafeZoneConfig.json";

    // First boot without a TraderX safezone config: start from the TraderPlus one if present.
    bool ImportTraderPlus()
    {
        if (!FileExist(TRADERPLUS_SAFEZONE_CONFIG))
            return false;

        TXSZTraderPlusConfig old = new TXSZTraderPlusConfig();
        string error;
        if (!JsonFileLoader<TXSZTraderPlusConfig>.LoadFile(TRADERPLUS_SAFEZONE_CONFIG, old, error))
            return false;

        version = "1.0.0";
        enableAfkDisconnect = old.EnableAfkDisconnect;
        kickAfterDelay = old.KickAfterDelay;
        cleanupTimer = old.CleanUpTimer;
        if (old.MsgEnterZone != string.Empty)
            msgOnEnteringZone = old.MsgEnterZone;
        if (old.MsgExitZone != string.Empty)
            msgOnLeavingZone = old.MsgExitZone;
        if (old.MsgOnLeavingZone != string.Empty)
            msgOnLeavingZoneCountdown = old.MsgOnLeavingZone + " %1";
        if (old.SZSteamUIDs)
            adminSteamUIDs.Copy(old.SZSteamUIDs);
        if (old.ObjectsToDelete)
            objectsToDelete.Copy(old.ObjectsToDelete);

        array<string> allowedAnimals = new array<string>();
        if (old.WhitelistEntities)
            allowedAnimals.Copy(old.WhitelistEntities);

        int id = 1;
        if (old.SafeAreaLocation)
        {
            foreach (TXSZTraderPlusZone zone : old.SafeAreaLocation)
            {
                if (!zone)
                    continue;
                array<string> zoneAnimals = new array<string>();
                zoneAnimals.Copy(allowedAnimals);
                locations.Insert(new TXSafeZoneLocation(true, id, zone.SafeZoneStatut, Vector(zone.X, 0, zone.Y), Math.Round(zone.Radius), true, zoneAnimals));
                // TraderPlus counts down per zone; TraderX has one countdown for every zone.
                if (zone.Countdown > 0)
                    leaveZoneCountdown = zone.Countdown;
                id++;
            }
        }

        CheckDirectories();
        SaveSettings();
        GetTXSZLog().LogInfo("[TXSZ Migration] Imported " + locations.Count() + " TraderPlus safezone(s) (stash/hideout not migrated)");
        return true;
    }
}
