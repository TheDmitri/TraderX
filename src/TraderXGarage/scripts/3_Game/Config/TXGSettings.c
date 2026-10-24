// TraderXGarage - Server configuration
// JSON-serializable, loaded from $profile:TraderX/TraderXGarage/TraderXGarageConfig.json.

class TXGNpcEntry
{
    int    npcId;           // matches the TraderX NPC id (GetNpcId) the player interacts with
    string className;       // NPC / building class spawned in standalone mode
    vector position;        // NPC world position (standalone spawn + nearest-fallback resolution)
    vector orientation;
    string name;
    // Vehicle slots belonging to THIS NPC. Retrieved vehicles spawn at the first free spot,
    // tied to the npcId the player clicked — not a global "nearest" position.
    ref array<ref TXGParkSpot> parkSpots;

    void TXGNpcEntry(int _npcId = -1, string _className = "", vector _position = vector.Zero, vector _orientation = vector.Zero, string _name = "")
    {
        npcId = _npcId;
        className = _className;
        position = _position;
        orientation = _orientation;
        name = _name;
        parkSpots = new array<ref TXGParkSpot>();
    }
}

class TXGCurrencyEntry
{
    string className;
    int    value;

    void TXGCurrencyEntry(string _className = "", int _value = 0)
    {
        className = _className;
        value = _value;
    }
}

class TXGInsuranceSettings
{
    bool   enabled = false;
    string recoveryMode = "INSURED_ONLY";       // "NONE" | "INSURED_ONLY" | "ALL_OWNED"
    float  premiumPercentPerDay = 2.0;          // % of coveredValue per day
    int    minDays = 1;
    int    maxDays = 30;
    float  deductiblePercent = 25.0;            // % of coveredValue
    int    deductibleMin = 500;
    int    claimCooldownHours = 24;
    int    maxActivePoliciesPerPlayer = 3;
    int    replacementNoResaleHours = 72;
    string replacementSpec = "CATALOG_PRESET";  // "CATALOG_PRESET" | "BARE"
    bool   holdCountsAgainstSlots = false;
    ref array<string> insurableClasses;

    void TXGInsuranceSettings()
    {
        insurableClasses = new array<string>();
    }
}

class TXGVehiclePriceEntry
{
    string className;
    int    buyPrice;
    int    sellPrice;

    void TXGVehiclePriceEntry(string _className = "", int _buyPrice = 0, int _sellPrice = 0)
    {
        className = _className;
        buyPrice = _buyPrice;
        sellPrice = _sellPrice;
    }
}

class TXGZoneEntry
{
    string name;
    vector position;
    float  radius;

    void TXGZoneEntry(string _name = "", vector _position = vector.Zero, float _radius = 0)
    {
        name = _name;
        position = _position;
        radius = _radius;
    }
}

// Vehicle receipts: a vehicle packed into an item (TraderPlus V1 "receipts"). The full vehicle state is
// kept server side (TraderXGarage/Receipts/<id>.json); the item only carries its id and display class.
class TXGReceiptSettings
{
    bool  enabled = false;          // players can pack vehicles (taking a vehicle out of a receipt always works)
    bool  traderOnly = true;        // pack and take out only near a trader or garage
    float traderDistance = 50.0;    // TraderPlus used 50 m
    bool  saveCargo = true;         // false: only empty vehicles can be packed
    bool  saveLock = true;          // keep the CarLock code (the player taking the vehicle out becomes the owner)
    float deployDistance = 6.0;     // the vehicle appears this far in front of the player
    vector deployBoxSize = "3 3 6"; // must be free of objects ('0 0 0' = no check)

    // TraderPlus V1 server: start with receipts on and its receipt options (TraderPlusGeneralConfig.json)
    void ImportTraderPlus()
    {
        string path = "$profile:TraderPlus/TraderPlusConfig/TraderPlusGeneralConfig.json";
        if (!FileExist(path))
            return;

        TXGTraderPlusReceiptOptions old = new TXGTraderPlusReceiptOptions();
        string error;
        if (!JsonFileLoader<TXGTraderPlusReceiptOptions>.LoadFile(path, old, error))
            return;

        enabled = true;
        traderOnly = old.IsReceiptTraderOnly;
        saveCargo = old.IsReceiptSaveCargo;
        saveLock = old.IsReceiptSaveLock;
    }
}

class TXGTraderPlusReceiptOptions
{
    bool IsReceiptTraderOnly = true;
    bool IsReceiptSaveLock;
    bool IsReceiptSaveCargo;
}

class TXGSettings
{
    // Inline help — serialized at the top of the JSON so the file explains itself.
    ref TStringArray __README;

    string version = TXGConstants.CONFIG_VERSION;
    bool   enableLogs = true;
    int    parkInCost = 100;
    int    parkOutCost = 100;
    int    maxVehicleStored = 5;
    bool   vehicleMustHaveLock = false;
    bool   saveVehicleLock = true;
    bool   saveVehicleCargo = true;
    bool   saveVehicleHealth = true;
    bool   saveVehicleFuel = true;
    bool   payWithBank = true;      // prefer banking provider; falls back to cash
    float  parkRadius = 15.0;       // search radius for a vehicle to park in
    float  parkClaimDistance = 6.0; // how close the player must be to an NPC park spot

    // Per-category repair pricing
    bool   enableRepairs = true;
    int    repairFluidCost = 100;
    int    repairBodyCost = 300;
    int    repairBodyPercent = 10;
    int    repairWornPartCost = 150;
    int    repairRuinedPartCost = 300;

    // Admin, Impound & Zones
    int    impoundFee = 500;
    ref array<string> adminSteamIds = new array<string>();
    ref array<ref TXGZoneEntry> noRetrieveZones = new array<ref TXGZoneEntry>();

    // Vehicle receipts (TraderPlus V1 compatible)
    ref TXGReceiptSettings receipts = new TXGReceiptSettings();

    // Vehicle Insurance block (see docs/handoff/INSURANCE_SPEC.md)
    ref TXGInsuranceSettings insurance = new TXGInsuranceSettings();
    // Standalone fallback vehicle catalog values (used when TraderX core catalog is absent)
    ref array<ref TXGVehiclePriceEntry> vehicleValues = new array<ref TXGVehiclePriceEntry>();

    ref array<ref TXGNpcEntry> npcs = new array<ref TXGNpcEntry>();
    // Cash fallback currency table (used only when banking is unavailable).
    ref array<ref TXGCurrencyEntry> currencies = new array<ref TXGCurrencyEntry>();

    void TXGSettings()
    {
        if (!__README)
            __README = new TStringArray();
        if (__README.Count() == 0)
        {
            __README.Insert("=== TraderXGarage config — ONE place to configure everything. Each 'npcs' entry is ONE garage. ===");
            __README.Insert("The garage spawns AND tags its own attendant in BOTH modes (TraderX or standalone). You do NOT add anything in TraderX's config.");
            __README.Insert("npcId: 0-127. Identifies the attendant and ties it to its parkSpots. Keep each garage's npcId unique.");
            __README.Insert("className: the NPC/object spawned (e.g. a survivor or a sign). position/orientation: where it stands.");
            __README.Insert("parkSpots: 1+ slots where retrieved vehicles appear. size = collision box 'x y z' ('0 0 0' = no check). First FREE slot is used.");
            __README.Insert("On server startup the log prints a TraderXGarage summary + warnings for bad entries. Read it.");
        }

        if (currencies.Count() == 0)
        {
            currencies.Insert(new TXGCurrencyEntry("TraderX_Money_Ruble5000", 5000));
            currencies.Insert(new TXGCurrencyEntry("TraderX_Money_Ruble2000", 2000));
            currencies.Insert(new TXGCurrencyEntry("TraderX_Money_Ruble1000", 1000));
            currencies.Insert(new TXGCurrencyEntry("TraderX_Money_Ruble500", 500));
            currencies.Insert(new TXGCurrencyEntry("TraderX_Money_Ruble200", 200));
            currencies.Insert(new TXGCurrencyEntry("TraderX_Money_Ruble100", 100));
            currencies.Insert(new TXGCurrencyEntry("TraderX_Money_Ruble50", 50));
            currencies.Insert(new TXGCurrencyEntry("TraderX_Money_Ruble10", 10));
            currencies.Insert(new TXGCurrencyEntry("TraderX_Money_Ruble5", 5));
            currencies.Insert(new TXGCurrencyEntry("TraderX_Money_Ruble2_Coin", 2));
            currencies.Insert(new TXGCurrencyEntry("TraderX_Money_Ruble1_Coin", 1));
        }

        if (npcs.Count() == 0)
        {
            TXGNpcEntry e = new TXGNpcEntry(TXGConstants.GARAGE_NPC_ID, "SurvivorM_Mirek", Vector(11632.63, 58.13, 14658.4), Vector(250, 0, 0), "Garage");
            e.parkSpots.Insert(new TXGParkSpot(Vector(11633.36, 57.81, 14650.17), Vector(250, 0, 0), Vector(6, 3, 12)));
            npcs.Insert(e);
        }
    }

    static bool IsChernarusWorld()
    {
        if (!GetGame())
            return true;
        string worldName = "";
        GetGame().GetWorldName(worldName);
        worldName.ToLower();
        if (worldName == "chernarusplus" || worldName == "chernarus")
            return true;
        return false;
    }

    bool IsCurrency(string _className)
    {
        foreach (TXGCurrencyEntry entry : currencies)
        {
            if (entry.className == _className)
                return true;
        }
        return false;
    }

    int GetCurrencyValue(string _className)
    {
        foreach (TXGCurrencyEntry entry : currencies)
        {
            if (entry.className == _className)
                return entry.value;
        }
        return 0;
    }

    array<ref TXGCurrencyEntry> GetCurrenciesSortedAsc()
    {
        array<ref TXGCurrencyEntry> sorted = new array<ref TXGCurrencyEntry>();
        foreach (TXGCurrencyEntry entry : currencies)
        {
            sorted.Insert(entry);
        }

        for (int i = 1; i < sorted.Count(); i++)
        {
            ref TXGCurrencyEntry key = sorted[i];
            int j = i - 1;
            while (j >= 0 && sorted[j].value > key.value)
            {
                sorted[j + 1] = sorted[j];
                j = j - 1;
            }
            sorted[j + 1] = key;
        }

        return sorted;
    }

    void Save()
    {
        if (!FileExist(TXGConstants.ROOT_DIR))
            MakeDirectory(TXGConstants.ROOT_DIR);
        if (!FileExist(TXGConstants.CONFIG_DIR))
            MakeDirectory(TXGConstants.CONFIG_DIR);

        JsonFileLoader<TXGSettings>.JsonSaveFile(TXGConstants.CONFIG_FILE, this);
        GetTXGLog().LogInfo("[TXG] Config saved to " + TXGConstants.CONFIG_FILE);
    }

    // Config 1.0 (TraderX stable) had one parkPosition/parkOrientation per NPC and no npcId.
    // Carry them into parkSpots and give each garage a distinct id (first = the reserved garage id).
    void UpgradeLegacyNpcs()
    {
        TXGLegacySettings legacy = new TXGLegacySettings();
        JsonFileLoader<TXGLegacySettings>.JsonLoadFile(TXGConstants.CONFIG_FILE, legacy);
        if (!legacy.npcs)
            return;

        for (int i = 0; i < npcs.Count() && i < legacy.npcs.Count(); i++)
        {
            TXGNpcEntry npc = npcs[i];
            TXGLegacyNpcEntry old = legacy.npcs[i];
            if (!npc || !old)
                continue;
            if (npc.npcId <= 0)
                npc.npcId = TXGConstants.GARAGE_NPC_ID - i;
            if (!npc.parkSpots)
                npc.parkSpots = new array<ref TXGParkSpot>();
            if (npc.parkSpots.Count() == 0 && old.parkPosition != vector.Zero)
                npc.parkSpots.Insert(new TXGParkSpot(old.parkPosition, old.parkOrientation, Vector(6, 3, 12)));
            GetTXGLog().LogInfo("[TXG] Config 1.0 upgrade: garage '" + npc.name + "' npcId=" + npc.npcId.ToString() + " parkSpots=" + npc.parkSpots.Count().ToString());
        }
    }

    static ref TXGSettings Load()
    {
        TXGSettings settings;

        if (!FileExist(TXGConstants.ROOT_DIR))
            MakeDirectory(TXGConstants.ROOT_DIR);
        if (!FileExist(TXGConstants.CONFIG_DIR))
            MakeDirectory(TXGConstants.CONFIG_DIR);

        if (FileExist(TXGConstants.CONFIG_FILE))
        {
            settings = new TXGSettings();
            JsonFileLoader<TXGSettings>.JsonLoadFile(TXGConstants.CONFIG_FILE, settings);
            GetTXGLog().LogInfo("[TXG] Config loaded from " + TXGConstants.CONFIG_FILE);

            if (!settings.insurance)
                settings.insurance = new TXGInsuranceSettings();
            if (!settings.insurance.insurableClasses)
                settings.insurance.insurableClasses = new array<string>();
            if (!settings.adminSteamIds)
                settings.adminSteamIds = new array<string>();
            if (!settings.noRetrieveZones)
                settings.noRetrieveZones = new array<ref TXGZoneEntry>();
            if (!settings.vehicleValues)
                settings.vehicleValues = new array<ref TXGVehiclePriceEntry>();
            if (!settings.receipts)
                settings.receipts = new TXGReceiptSettings();

            if (settings.version == "1.0")
                settings.UpgradeLegacyNpcs();

            if (settings.version != TXGConstants.CONFIG_VERSION)
            {
                settings.version = TXGConstants.CONFIG_VERSION;
                settings.Save();
            }
        }
        else
        {
            settings = new TXGSettings();
            settings.receipts.ImportTraderPlus();
            settings.Save();
            GetTXGLog().LogInfo("[TXG] Default config created at " + TXGConstants.CONFIG_FILE);
        }

        return settings;
    }
}

class TXGLegacyNpcEntry
{
    vector parkPosition;
    vector parkOrientation;
}

class TXGLegacySettings
{
    ref array<ref TXGLegacyNpcEntry> npcs;
}
