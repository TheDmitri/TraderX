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

        // TraderPlus may still be loaded on this server: never touch its own config folder.
        // The TraderX-side config written by this migration already prevents a re-run.
        if (sourcePath.IndexOf("$profile:TraderPlus/") == 0)
        {
            GetTXGLog().LogInfo("[TXG Migration] Imported " + sourcePath + " (left in place)");
            return;
        }

        string deletedDir = TXGConstants.CONFIG_DIR + "deleted/";
        if (!FileExist(deletedDir))
            MakeDirectory(deletedDir);

        string dest = deletedDir + "TraderPlusGarageConfig.json";
        CopyFile(sourcePath, dest);
        if (!FileExist(dest))
        {
            GetTXGLog().LogError("[TXG Migration] Could not archive " + sourcePath + " to " + dest + " - keeping the original");
            return;
        }

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

        TXGTraderPlusCurrencyImport.Apply(settings.currencies, null);
        settings.Save();
        GetTXGLog().LogInfo("[TXG Migration] Converted TraderPlus garage config: " + settings.npcs.Count() + " NPC(s), parkIn=" + settings.parkInCost + " parkOut=" + settings.parkOutCost + " maxStored=" + settings.maxVehicleStored);
        if (NPCs.Count() > 1)
            GetTXGLog().LogInfo("[TXG Migration] WARNING: " + NPCs.Count() + " garage NPCs migrated with auto-assigned ids (110, 111, 112 ...). Verify they don't collide with your trader/bank NPC ids.");
    }

    static void ConvertIfPresent()
    {
        // Never clobber an existing TraderXGarage config.
        if (FileExist(TXGConstants.CONFIG_FILE))
        {
            // A previous import could not list the TraderPlus garages: retry only the stored vehicles.
            if (FileExist(TXGTraderPlusGarageImport.PENDING_FILE))
                TXGTraderPlusGarageImport.ImportAll();
            return;
        }

        string path = ResolveSource();
        if (path == string.Empty)
            return;

        GetTXGLog().LogInfo("[TXG Migration] Loading legacy garage config: " + path);
        oldTraderPlusGarageConfig old = new oldTraderPlusGarageConfig();
        JsonFileLoader<oldTraderPlusGarageConfig>.JsonLoadFile(path, old);
        old.ConvertAndSave();
        TXGTraderPlusGarageImport.ImportAll();
        ArchiveSource(path);
    }
}

// TraderPlus currencies live in TraderPlusGeneralConfig.json: [{"ClassName": "a,b", "Value": n}].
class TXGTraderPlusCurrency
{
    string ClassName;
    int    Value;
}

class TXGTraderPlusCurrencyList
{
    ref array<ref TXGTraderPlusCurrency> Currencies;
}

class TXGTraderPlusCurrencyImport
{
    static const string GENERAL_CONFIG = "$profile:TraderPlus/TraderPlusConfig/TraderPlusGeneralConfig.json";

    // Replaces the default TraderX_Money_* list with the notes TraderPlus players actually hold.
    static void Apply(array<ref TXGCurrencyEntry> currencies, array<string> acceptedOnly)
    {
        if (!FileExist(GENERAL_CONFIG))
            return;

        TXGTraderPlusCurrencyList list = new TXGTraderPlusCurrencyList();
        string error;
        if (!JsonFileLoader<TXGTraderPlusCurrencyList>.LoadFile(GENERAL_CONFIG, list, error) || !list.Currencies || list.Currencies.Count() == 0)
            return;

        currencies.Clear();
        foreach (TXGTraderPlusCurrency currency : list.Currencies)
        {
            TStringArray classNames = new TStringArray();
            currency.ClassName.Split(",", classNames);
            foreach (string className : classNames)
            {
                className.TrimInPlace();
                if (className == string.Empty)
                    continue;
                if (acceptedOnly && acceptedOnly.Count() > 0 && acceptedOnly.Find(className) == -1)
                    continue;
                currencies.Insert(new TXGCurrencyEntry(className, currency.Value));
            }
        }
        GetTXGLog().LogInfo("[TXG Migration] Imported " + currencies.Count() + " TraderPlus currency class(es)");
    }
}

// ---------------------------------------------------------------------------------------------
// TraderPlus stored vehicles -> TraderXGarage.
// TraderPlus files: TraderPlusGarageDatabase/Garage_<lowID>.json (+ <lowID>/<cargoId>.json trees),
// where lowID = digits 8..16 of the steam64 id. Owners are resolved through the TraderPlus bank
// accounts (Account_<steam64>.json); unmatched ids fall back to the 76561198 prefix.
// ---------------------------------------------------------------------------------------------
class TXGTraderPlusCargoNode
{
    string item;
    int    quantity;
    int    slot;
    int    magAmmoCount;
    int    liquidType;
    int    foodstage;
    int    extraData;
    float  temp;
    float  wetness;
    float  health;
    float  energy;
    int    locType;     // InventoryLocationType: 2 = attachment, 3 = cargo
    ref array<ref TXGTraderPlusCargoNode> childrens;
}

class TXGTraderPlusLegacyItem
{
    string ItemName;
    int    Amount;
    float  Health;
    int    ExtraData;
}

class TXGTraderPlusVehicle
{
    string VehicleName;
    int    VehicleCargoId;
    int    VehicleID;
    float  EngineHealth;
    float  FuelAmmount;
    ref array<ref TXGTraderPlusLegacyItem> ItemsInCargo;
}

class TXGTraderPlusGarage
{
    string OwnerName;
    string OwnerID;
    ref array<ref TXGTraderPlusVehicle> Vehicles;
}

class TXGTraderPlusAccountId
{
    string SteamID64;
    string Name;
}

class TXGTraderPlusGarageImport
{
    static const string TP_GARAGE_DIR = "$profile:TraderPlus/TraderPlusGarageDatabase/";
    static const string TP_BANK_DIR = "$profile:TraderPlus/TraderPlusBankDatabase/";
    // Present while the stored vehicles still have to be imported (FindFile could not list them).
    static const string PENDING_FILE = "$profile:TraderX/TraderXGarage/.pending_traderplus_garage";

    static string LowId(string steam64)
    {
        if (steam64.Length() < 17)
            return string.Empty;
        return steam64.Substring(8, 9).ToInt().ToString();
    }

    static void ImportAll()
    {
        map<string, string> steam64ByLowId = BuildLowIdMap();

        string fileName;
        FileAttr attr;
        FindFileHandle handle = TXGFileSystem.Find(TP_GARAGE_DIR + "Garage_*.json", fileName, attr, FindFileFlags.ALL);
        if (!handle)
        {
            if (FileExist(TP_GARAGE_DIR))
                MarkPending();
            return;
        }

        int garages = 0;
        int vehicles = 0;
        bool hasNext = true;
        while (hasNext)
        {
            vehicles += ImportGarage(TP_GARAGE_DIR + fileName, steam64ByLowId);
            garages++;
            hasNext = FindNextFile(handle, fileName, attr);
        }
        CloseFindFile(handle);

        GetTXGLog().LogInfo("[TXG Migration] Imported " + vehicles + " stored vehicle(s) from " + garages + " TraderPlus garage(s)");
        if (FileExist(PENDING_FILE))
            DeleteFile(PENDING_FILE);
    }

    // The TraderPlus garage folder exists but FindFile listed nothing (DayZ 1.30 with an absolute or
    // out-of-tree -profiles path). Nothing was imported, so a retry on the next boot cannot duplicate vehicles.
    protected static void MarkPending()
    {
        string message = "stored vehicles in " + TP_GARAGE_DIR + ": FindFile could not list the folder (DayZ 1.30 with an absolute -profiles path?). Use a -profiles path relative to the server folder and restart - the import retries while " + PENDING_FILE + " exists (delete it if the folder is really empty).";
        GetTXGLog().LogError("[TXG Migration] " + message);
#ifdef TraderX
        TraderXMigrationReport.Skipped("TraderPlusGarage", message);
        TraderXMigrationReport.Write("TraderPlusGarage");
#endif

        if (!FileExist(TXGConstants.ROOT_DIR))
            MakeDirectory(TXGConstants.ROOT_DIR);
        if (!FileExist(TXGConstants.CONFIG_DIR))
            MakeDirectory(TXGConstants.CONFIG_DIR);
        FileHandle fh = OpenFile(PENDING_FILE, FileMode.WRITE);
        if (fh)
        {
            FPrintln(fh, "TraderPlus stored vehicles not imported yet - retried on every boot until FindFile can list " + TP_GARAGE_DIR);
            CloseFile(fh);
        }
    }

    protected static map<string, string> BuildLowIdMap()
    {
        map<string, string> result = new map<string, string>();
        string fileName;
        FileAttr attr;
        FindFileHandle handle = TXGFileSystem.Find(TP_BANK_DIR + "Account_*.json", fileName, attr, FindFileFlags.ALL);
        if (!handle)
            return result;

        bool hasNext = true;
        while (hasNext)
        {
            TXGTraderPlusAccountId account = new TXGTraderPlusAccountId();
            string error;
            if (JsonFileLoader<TXGTraderPlusAccountId>.LoadFile(TP_BANK_DIR + fileName, account, error) && account.SteamID64 != string.Empty)
                result.Set(LowId(account.SteamID64), account.SteamID64);
            hasNext = FindNextFile(handle, fileName, attr);
        }
        CloseFindFile(handle);
        return result;
    }

    protected static string ResolveSteam64(string lowId, map<string, string> steam64ByLowId)
    {
        if (steam64ByLowId.Contains(lowId))
            return steam64ByLowId.Get(lowId);

        string padded = lowId;
        while (padded.Length() < 9)
            padded = "0" + padded;
        string guessed = "76561198" + padded;
        GetTXGLog().LogWarning("[TXG Migration] TraderPlus garage owner " + lowId + " has no TraderPlus bank account - assumed steam64 " + guessed);
        return guessed;
    }

    protected static int ImportGarage(string path, map<string, string> steam64ByLowId)
    {
        TXGTraderPlusGarage garage = new TXGTraderPlusGarage();
        string error;
        if (!JsonFileLoader<TXGTraderPlusGarage>.LoadFile(path, garage, error) || !garage.Vehicles || garage.OwnerID == string.Empty)
        {
            GetTXGLog().LogWarning("[TXG Migration] Could not read TraderPlus garage " + path + " " + error);
            return 0;
        }

        string owner = ResolveSteam64(garage.OwnerID, steam64ByLowId);
        TXGGarageData data = TXGGarageData.Load(owner, garage.OwnerName);
        int imported = 0;
        foreach (TXGTraderPlusVehicle vehicle : garage.Vehicles)
        {
            if (!vehicle || vehicle.VehicleName == string.Empty)
                continue;
            data.vehicles.Insert(ConvertVehicle(vehicle, garage.OwnerID, data));
            imported++;
        }
        data.Save();
        return imported;
    }

    protected static TXGVehicleData ConvertVehicle(TXGTraderPlusVehicle vehicle, string lowId, TXGGarageData data)
    {
        TXGVehicleData record = new TXGVehicleData();
        record.vehicleId = data.AllocateVehicleId();
        record.className = vehicle.VehicleName;
        record.health = vehicle.EngineHealth;
        record.timestamp = 0;
        record.lockCode = 0;

        float capacity = GetGame().ConfigGetFloat("CfgVehicles " + vehicle.VehicleName + " fuelCapacity");
        record.fuelFraction = 0;
        if (capacity > 0)
            record.fuelFraction = Math.Clamp(vehicle.FuelAmmount / capacity, 0, 1);

        record.snapshot = new TXGItemSnapshot();
        record.snapshot.item = vehicle.VehicleName;

        if (vehicle.VehicleCargoId > 0)
        {
            TXGTraderPlusCargoNode root = new TXGTraderPlusCargoNode();
            string cargoPath = TP_GARAGE_DIR + lowId + "/" + vehicle.VehicleCargoId.ToString() + ".json";
            string error;
            if (JsonFileLoader<TXGTraderPlusCargoNode>.LoadFile(cargoPath, root, error) && root.childrens)
            {
                foreach (TXGTraderPlusCargoNode child : root.childrens)
                {
                    record.snapshot.childrens.Insert(ConvertNode(child));
                }
            }
            else
            {
                GetTXGLog().LogWarning("[TXG Migration] Missing cargo file " + cargoPath + " for " + vehicle.VehicleName);
            }
        }
        else if (vehicle.ItemsInCargo)
        {
            // Old TraderPlus format: a flat list, attachments are re-fitted on retrieval.
            foreach (TXGTraderPlusLegacyItem item : vehicle.ItemsInCargo)
            {
                TXGItemSnapshot snap = new TXGItemSnapshot();
                snap.item = item.ItemName;
                snap.quantity = item.Amount;
                snap.health = item.Health;
                snap.extraData = item.ExtraData;
                snap.slot = -1;
                snap.isAttachment = false;
                record.snapshot.childrens.Insert(snap);
            }
        }

        return record;
    }

    protected static TXGItemSnapshot ConvertNode(TXGTraderPlusCargoNode node)
    {
        TXGItemSnapshot snap = new TXGItemSnapshot();
        snap.item = node.item;
        snap.quantity = node.quantity;
        snap.slot = node.slot;
        snap.magAmmoCount = node.magAmmoCount;
        snap.liquidType = node.liquidType;
        snap.foodstage = node.foodstage;
        snap.extraData = node.extraData;
        snap.temp = node.temp;
        snap.wetness = node.wetness;
        snap.health = node.health;
        snap.energy = node.energy;
        snap.isAttachment = (node.locType == 2);

        if (node.childrens)
        {
            foreach (TXGTraderPlusCargoNode child : node.childrens)
            {
                snap.childrens.Insert(ConvertNode(child));
            }
        }
        return snap;
    }
}
