// TraderPlus V1 -> TraderXGarage config migration.
// Self-contained inside the TraderXGarage PBO (no TraderX-core class refs) so it also works
// in standalone mode where the core TraderX mod is not loaded.
//
// Runs once, before TXGSettings.Load(): if no TraderXGarage config exists yet AND a TraderPlus
// garage config is found, convert it, write TraderXGarageConfig.json, then archive the source.

class oldTraderPlusGarageNpc
{
    string ClassName;
    vector Position;
    vector Orientation;
    vector ParkingPosition;
    vector ParkingOrientation;
    ref array<string> Clothes;

    void oldTraderPlusGarageNpc()
    {
        Clothes = new array<string>();
    }
}

class oldTraderPlusGarageConfig
{
    string Version = "";
    int    UseGarageOnlyToTrade;
    int    VehicleMustHaveLock;
    int    SaveVehicleLock;
    int    SaveVehicleCargo;
    int    SaveVehicleHealth;
    int    SaveVehicleFuel;
    int    SavedVehicleInGarageForTradeInHour;
    int    MaxVehicleStored;
    int    ParkInCost;
    int    ParkOutCost;
    int    PayWithBankAccount;
    ref array<string> WhitelistedObjects;
    ref array<ref oldTraderPlusGarageNpc> NPCs;

    void oldTraderPlusGarageConfig()
    {
        WhitelistedObjects = new array<string>();
        NPCs = new array<ref oldTraderPlusGarageNpc>();
    }

    // Candidate source locations, in priority order.
    static string ResolveSource()
    {
        string candidate;

        candidate = TXGConstants.CONFIG_DIR + "TraderPlusGarageConfig.json";
        if (FileExist(candidate))
            return candidate;

        candidate = "$profile:TraderPlus/TraderPlusConfig/TraderPlusGarageConfig.json";
        if (FileExist(candidate))
            return candidate;

        return string.Empty;
    }

    static void ArchiveSource(string sourcePath)
    {
        if (sourcePath == string.Empty || !FileExist(sourcePath))
            return;

        string deletedDir = TXGConstants.CONFIG_DIR + "deleted/";
        if (!FileExist(deletedDir))
            MakeDirectory(deletedDir);

        string dest = deletedDir + "TraderPlusGarageConfig.json";
        CopyFile(sourcePath, dest);
        DeleteFile(sourcePath);
        GetTXGLog().LogInfo("[TXG Migration] Archived consumed legacy garage config: " + sourcePath + " -> " + dest);
    }

    void ConvertAndSave()
    {
        TXGSettings settings = new TXGSettings();

        settings.parkInCost       = ParkInCost;
        settings.parkOutCost      = ParkOutCost;
        settings.maxVehicleStored = MaxVehicleStored;
        settings.saveVehicleCargo  = (SaveVehicleCargo != 0);
        settings.saveVehicleHealth = (SaveVehicleHealth != 0);
        settings.saveVehicleFuel   = (SaveVehicleFuel != 0);
        settings.payWithBank       = (PayWithBankAccount != 0);

        settings.npcs.Clear();

        int index = 0;
        foreach (oldTraderPlusGarageNpc oldNpc : NPCs)
        {
            // NPC ids must stay unique in 0-127. A single migrated garage keeps the conventional
            // GARAGE_NPC_ID; extras get a high-but-safe band the admin can re-number later.
            int npcId = 110 + index;
            if (NPCs.Count() == 1)
                npcId = TXGConstants.GARAGE_NPC_ID;

            TXGNpcEntry entry = new TXGNpcEntry(npcId, oldNpc.ClassName, oldNpc.Position, oldNpc.Orientation, "Garage");
            entry.parkSpots.Insert(new TXGParkSpot(oldNpc.ParkingPosition, oldNpc.ParkingOrientation, Vector(6, 3, 12)));
            settings.npcs.Insert(entry);
            index++;
        }

        settings.Save();
        GetTXGLog().LogInfo("[TXG Migration] Converted TraderPlus garage config: " + settings.npcs.Count() + " NPC(s), parkIn=" + settings.parkInCost + " parkOut=" + settings.parkOutCost + " maxStored=" + settings.maxVehicleStored);
        if (NPCs.Count() > 1)
            GetTXGLog().LogInfo("[TXG Migration] WARNING: " + NPCs.Count() + " garage NPCs migrated with auto-assigned ids (110, 111, 112 ...). Verify they don't collide with your trader/bank NPC ids.");
    }

    static void ConvertIfPresent()
    {
        // Never clobber an existing TraderXGarage config.
        if (FileExist(TXGConstants.CONFIG_FILE))
            return;

        string path = ResolveSource();
        if (path == string.Empty)
            return;

        GetTXGLog().LogInfo("[TXG Migration] Loading legacy garage config: " + path);
        oldTraderPlusGarageConfig old = new oldTraderPlusGarageConfig();
        JsonFileLoader<oldTraderPlusGarageConfig>.JsonLoadFile(path, old);
        old.ConvertAndSave();
        ArchiveSource(path);
    }
}
