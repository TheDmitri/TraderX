// Dr Jones Trader (TM/Trader, Workshop 1590841260) -> TraderX converter (one-shot, sentinel
// ".migrated_drjones", never overwrites). Sources in $profile:Trader/ (files chain with <OpenFile>):
//   TraderConfig.txt        <CurrencyName>/<Currency>, <Trader> (id = order), <Category>, "Class, Qty, Buy, Sell"
//   TraderObjects.txt       <TraderMarker> id/position/safezone radius, <VehicleSpawn>, then <Object> = the NPC
//                           (position, orientation, <ObjectAttachment> = clothes); later objects = decoration
//   TraderVehicleParts.txt  <VehicleParts> Vehicle + part lines (parts fitted when the vehicle is bought)
//   TraderVariables.txt     <SafezoneTimeout> etc.
//   TraderAdmins.txt        vehicle-lock admins (no TraderX equivalent)
// Quantity codes: * = full, number = fixed amount, M = magazine, W = weapon, S = steak,
// V/VNK = vehicle with/without key, K = key duplicate. Buy/Sell -1 = not buyable/sellable.

class DrJonesProductLine
{
    string className;
    string quantity;
    int buyPrice;
    int sellPrice;
}

class DrJonesCategory
{
    string name;
    ref array<ref DrJonesProductLine> lines = new array<ref DrJonesProductLine>();
}

class DrJonesTrader
{
    string name;
    ref array<ref DrJonesCategory> categories = new array<ref DrJonesCategory>();
}

class DrJonesMarker
{
    int traderId = -1;
    vector position;
    float safezoneRadius;
    bool hasVehicleSpawn;
    vector vehicleSpawn;
    vector vehicleSpawnOri;
    string npcClass;
    vector npcPosition;
    vector npcOrientation;
    ref array<string> npcGear = new array<string>();
}

// Mirror of TraderXSafeZone's TXSafeZoneSettings JSON (core does not depend on the safezone PBO).
class DrJonesSafeZoneLocation
{
    int id;
    bool isActive = true;
    string name;
    vector position;
    int radius;
    bool isEntitiesCleanActive = true;
    ref array<string> allowedAnimals = new array<string>();
}

class DrJonesSafeZoneSettings
{
    string version = "1.0.0";
    int notificationTimer = 10;
    string notificationTitle = "SafeZone";
    string msgOnEnteringZone = "You entered the safezone!";
    string msgOnLeavingZone = "You have left the safezone!";
    int leaveZoneCountdown = 30;
    string msgOnLeavingZoneCountdown = "You are leaving the safezone in %1 seconds";
    ref array<string> adminSteamUIDs = new array<string>();
    bool enableAfkDisconnect = false;
    int kickAfterDelay = 300;
    int cleanupTimer = 1200;
    ref array<string> objectsToDelete = new array<string>();
    ref array<ref DrJonesSafeZoneLocation> locations = new array<ref DrJonesSafeZoneLocation>();
}

class ConvertDrJonesTrader
{
    static const string SRC = "DrJones";
    static const string DIR = "$profile:Trader/";
    static const string SAFEZONE_FILE = "$profile:TraderX/TraderXSafeZone/TXSafeZoneSettings.json";

    ref array<ref DrJonesTrader> m_Traders = new array<ref DrJonesTrader>();
    ref TraderXCurrencyType m_Currency;
    ref map<string, ref array<string>> m_VehicleParts = new map<string, ref array<string>>();
    ref array<ref DrJonesMarker> m_Markers = new array<ref DrJonesMarker>();
    ref array<ref TraderXObject> m_Decorations = new array<ref TraderXObject>();
    float m_SafezoneTimeout = -1;
    bool m_RemoveAnimals = true;
    // An existing TraderX file could not be read: later steps are skipped and no sentinel is written.
    bool m_LoadFailed;

    ref map<int, ref array<string>> m_CategoryIdsByTrader = new map<int, ref array<string>>();
    int m_ProductCount;

    static void ConvertIfPresent()
    {
        if (!FileExist(DIR + "TraderConfig.txt"))
            return;
        if (TraderXExternalMigration.IsDone("drjones"))
            return;

        ConvertDrJonesTrader converter = new ConvertDrJonesTrader();
        converter.Run();
    }

    void Run()
    {
        GetTraderXLogger().LogInfo("[TraderX Migration] Dr Jones Trader: importing " + DIR);
        TraderXExternalMigration.EnsureCatalogDirs();

        ParseConfig("TraderConfig.txt", null, null, 0);
        ParseVehicleParts("TraderVehicleParts.txt", null, 0);
        ParseObjects("TraderObjects.txt", 0);
        ParseVariables();
        if (FileExist(DIR + "TraderAdmins.txt"))
            TraderXMigrationReport.Skipped(SRC, "TraderAdmins.txt (admins who can unlock any bought vehicle)");

        BuildCatalog();
        SaveCurrency();
        // NPCs are appended, not merged: only import them once the earlier steps succeeded so a retry cannot duplicate them.
        if (!m_LoadFailed)
            ImportNpcs();

        if (m_LoadFailed)
        {
            TraderXMigrationReport.Error(SRC, "migration incomplete - not marked done, it will run again on next restart");
            TraderXMigrationReport.Write(SRC);
            return;
        }

        TraderXExternalMigration.MarkDone("drjones", "Dr Jones Trader migrated. traders=" + m_Traders.Count() + " products=" + m_ProductCount);
        TraderXMigrationReport.Write(SRC);
    }

    // ----------------------------------------------------------------------------- parsing

    protected void ParseConfig(string file, DrJonesTrader trader, DrJonesCategory category, int depth)
    {
        FileHandle fh = OpenFile(DIR + file, FileMode.READ);
        if (!fh)
        {
            TraderXMigrationReport.Skipped(SRC, "missing file " + file);
            return;
        }

        string raw;
        while (FGets(fh, raw) >= 0)
        {
            string line = TraderXExternalMigration.CleanLine(raw);
            if (line == string.Empty)
                continue;
            if (TraderXExternalMigration.StartsWith(line, "<FileEnd>"))
                break;
            if (TraderXExternalMigration.StartsWith(line, "<OpenFile>"))
            {
                // A linked file continues the current trader/category (that is how Dr Jones chains files).
                if (depth < 8)
                    ParseConfig(TraderXExternalMigration.AfterTag(line, "<OpenFile>"), trader, category, depth + 1);
                if (m_Traders.Count() > 0)
                    trader = m_Traders[m_Traders.Count() - 1];
                category = null;
                if (trader && trader.categories.Count() > 0)
                    category = trader.categories[trader.categories.Count() - 1];
                continue;
            }
            if (TraderXExternalMigration.StartsWith(line, "<CurrencyName>"))
            {
                string currencyName = TraderXExternalMigration.AfterTag(line, "<CurrencyName>");
                currencyName.Replace("#", "");
                currencyName.Replace("tm_", "");
                currencyName.ToUpper();
                m_Currency = new TraderXCurrencyType(currencyName);
                continue;
            }
            if (TraderXExternalMigration.StartsWith(line, "<Currency>"))
            {
                ParseCurrency(TraderXExternalMigration.AfterTag(line, "<Currency>"));
                continue;
            }
            if (TraderXExternalMigration.StartsWith(line, "<Trader>"))
            {
                trader = AddTrader(TraderXExternalMigration.AfterTag(line, "<Trader>"));
                category = null;
                continue;
            }
            if (TraderXExternalMigration.StartsWith(line, "<Category>"))
            {
                if (!trader)
                    continue;
                category = AddCategory(trader, TraderXExternalMigration.AfterTag(line, "<Category>"));
                continue;
            }
            if (category)
                ParseProductLine(line, category);
        }
        CloseFile(fh);
    }

    // Parameters are weak references on 1.30: objects are created and owned here, then handed back.
    protected DrJonesTrader AddTrader(string name)
    {
        DrJonesTrader trader = new DrJonesTrader();
        trader.name = name;
        m_Traders.Insert(trader);
        return trader;
    }

    protected DrJonesCategory AddCategory(DrJonesTrader trader, string name)
    {
        DrJonesCategory category = new DrJonesCategory();
        category.name = name;
        trader.categories.Insert(category);
        return category;
    }

    protected array<string> AddVehicleParts(string vehicle)
    {
        array<string> parts = new array<string>();
        m_VehicleParts.Set(vehicle, parts);
        return parts;
    }

    protected void ParseCurrency(string value)
    {
        if (!m_Currency)
            m_Currency = new TraderXCurrencyType("RUBLE");
        TStringArray parts = new TStringArray();
        value.Split(",", parts);
        if (parts.Count() < 2)
            return;
        string cls = parts[0];
        cls.TrimInPlace();
        string amount = parts[1];
        amount.TrimInPlace();
        m_Currency.AddCurrency(cls, amount.ToInt());
    }

    protected void ParseProductLine(string line, DrJonesCategory category)
    {
        TStringArray parts = new TStringArray();
        line.Split(",", parts);
        if (parts.Count() < 4)
            return;
        DrJonesProductLine product = new DrJonesProductLine();
        string cls = parts[0];
        cls.TrimInPlace();
        string qty = parts[1];
        qty.TrimInPlace();
        product.className = cls;
        product.quantity = qty;
        string buy = parts[2];
        buy.TrimInPlace();
        string sell = parts[3];
        sell.TrimInPlace();
        product.buyPrice = buy.ToInt();
        product.sellPrice = sell.ToInt();
        if (product.className != string.Empty)
            category.lines.Insert(product);
    }

    protected void ParseVehicleParts(string file, array<string> current, int depth)
    {
        FileHandle fh = OpenFile(DIR + file, FileMode.READ);
        if (!fh)
            return;

        string raw;
        while (FGets(fh, raw) >= 0)
        {
            string line = TraderXExternalMigration.CleanLine(raw);
            if (line == string.Empty)
                continue;
            if (TraderXExternalMigration.StartsWith(line, "<FileEnd>"))
                break;
            if (TraderXExternalMigration.StartsWith(line, "<OpenFile>"))
            {
                if (depth < 8)
                    ParseVehicleParts(TraderXExternalMigration.AfterTag(line, "<OpenFile>"), current, depth + 1);
                continue;
            }
            if (TraderXExternalMigration.StartsWith(line, "<VehicleParts>"))
            {
                string vehicle = TraderXExternalMigration.AfterTag(line, "<VehicleParts>");
                vehicle.ToLower();
                current = AddVehicleParts(vehicle);
                continue;
            }
            if (current)
                current.Insert(line);
        }
        CloseFile(fh);
    }

    protected void ParseObjects(string file, int depth)
    {
        FileHandle fh = OpenFile(DIR + file, FileMode.READ);
        if (!fh)
        {
            TraderXMigrationReport.Skipped(SRC, "TraderObjects.txt not found - traders have no NPC/position");
            return;
        }

        DrJonesMarker marker = null;
        TraderXObject decoration = null;
        string raw;
        while (FGets(fh, raw) >= 0)
        {
            string line = TraderXExternalMigration.CleanLine(raw);
            if (line == string.Empty)
                continue;
            if (TraderXExternalMigration.StartsWith(line, "<FileEnd>"))
                break;
            if (TraderXExternalMigration.StartsWith(line, "<OpenFile>"))
            {
                if (depth < 8)
                    ParseObjects(TraderXExternalMigration.AfterTag(line, "<OpenFile>"), depth + 1);
                continue;
            }
            if (TraderXExternalMigration.StartsWith(line, "<TraderMarker>"))
            {
                marker = new DrJonesMarker();
                marker.traderId = TraderXExternalMigration.AfterTag(line, "<TraderMarker>").ToInt();
                m_Markers.Insert(marker);
                decoration = null;
                continue;
            }
            if (TraderXExternalMigration.StartsWith(line, "<TraderMarkerPosition>") && marker)
            {
                marker.position = ParseVector(TraderXExternalMigration.AfterTag(line, "<TraderMarkerPosition>"));
                continue;
            }
            if (TraderXExternalMigration.StartsWith(line, "<TraderMarkerSafezone>") && marker)
            {
                marker.safezoneRadius = TraderXExternalMigration.AfterTag(line, "<TraderMarkerSafezone>").ToFloat();
                continue;
            }
            if (TraderXExternalMigration.StartsWith(line, "<VehicleSpawnOri>") && marker)
            {
                marker.vehicleSpawnOri = ParseVector(TraderXExternalMigration.AfterTag(line, "<VehicleSpawnOri>"));
                continue;
            }
            if (TraderXExternalMigration.StartsWith(line, "<VehicleSpawn>") && marker)
            {
                marker.hasVehicleSpawn = true;
                marker.vehicleSpawn = ParseVector(TraderXExternalMigration.AfterTag(line, "<VehicleSpawn>"));
                continue;
            }
            if (TraderXExternalMigration.StartsWith(line, "<Object>"))
            {
                string cls = TraderXExternalMigration.AfterTag(line, "<Object>");
                if (marker && marker.npcClass == string.Empty)
                {
                    marker.npcClass = cls;
                    decoration = null;
                }
                else
                {
                    marker = null;
                    decoration = new TraderXObject(cls, vector.Zero, vector.Zero);
                    m_Decorations.Insert(decoration);
                }
                continue;
            }
            if (TraderXExternalMigration.StartsWith(line, "<ObjectPosition>"))
            {
                vector objectPosition = ParseVector(TraderXExternalMigration.AfterTag(line, "<ObjectPosition>"));
                if (decoration)
                    decoration.position = objectPosition;
                else if (marker)
                    marker.npcPosition = objectPosition;
                continue;
            }
            if (TraderXExternalMigration.StartsWith(line, "<ObjectOrientation>"))
            {
                vector objectOrientation = ParseVector(TraderXExternalMigration.AfterTag(line, "<ObjectOrientation>"));
                if (decoration)
                    decoration.orientation = objectOrientation;
                else if (marker)
                    marker.npcOrientation = objectOrientation;
                continue;
            }
            if (TraderXExternalMigration.StartsWith(line, "<ObjectAttachment>") && marker && !decoration)
            {
                string gear = TraderXExternalMigration.AfterTag(line, "<ObjectAttachment>");
                if (gear != "NPC_DUMMY")
                    marker.npcGear.Insert(gear);
            }
        }
        CloseFile(fh);
    }

    protected void ParseVariables()
    {
        FileHandle fh = OpenFile(DIR + "TraderVariables.txt", FileMode.READ);
        if (!fh)
            return;
        string raw;
        while (FGets(fh, raw) >= 0)
        {
            string line = TraderXExternalMigration.CleanLine(raw);
            if (TraderXExternalMigration.StartsWith(line, "<SafezoneTimeout>"))
                m_SafezoneTimeout = TraderXExternalMigration.AfterTag(line, "<SafezoneTimeout>").ToFloat();
            else if (TraderXExternalMigration.StartsWith(line, "<SafezoneRemoveAnimals>"))
                m_RemoveAnimals = IsYes(TraderXExternalMigration.AfterTag(line, "<SafezoneRemoveAnimals>"));
            else if (TraderXExternalMigration.StartsWith(line, "<BuySellTimer>") || TraderXExternalMigration.StartsWith(line, "<VehicleCleanupTimer>") || TraderXExternalMigration.StartsWith(line, "<TradingDistance>") || TraderXExternalMigration.StartsWith(line, "<SafezoneRemoveEAI>"))
                TraderXMigrationReport.Skipped(SRC, "TraderVariables " + line);
        }
        CloseFile(fh);
    }

    // ----------------------------------------------------------------------------- catalog

    protected void BuildCatalog()
    {
        // Category names repeated across traders get the trader name appended to stay distinct.
        map<string, int> nameUse = new map<string, int>();
        foreach (DrJonesTrader t : m_Traders)
        {
            foreach (DrJonesCategory c : t.categories)
            {
                nameUse.Set(c.name, nameUse.Get(c.name) + 1);
            }
        }

        for (int traderId = 0; traderId < m_Traders.Count(); traderId++)
        {
            DrJonesTrader trader = m_Traders[traderId];
            array<string> categoryIds = new array<string>();
            m_CategoryIdsByTrader.Set(traderId, categoryIds);

            foreach (DrJonesCategory category : trader.categories)
            {
                string name = category.name;
                if (nameUse.Get(name) > 1)
                    name = name + " (" + trader.name + ")";
                string categoryId = TraderXCategoryId.GenerateCategoryId(name);
                TraderXCategory txCategory = TraderXCategory.CreateCategory(name, 0, categoryId);
                foreach (DrJonesProductLine line : category.lines)
                {
                    TraderXProduct product = CreateProduct(line);
                    if (product)
                        txCategory.AddProduct(product);
                }
                TraderXExternalMigration.SaveCategoryIfNew(txCategory);
                categoryIds.Insert(categoryId);
            }
        }
        TraderXMigrationReport.Info(SRC, m_Traders.Count().ToString() + " traders, " + m_ProductCount.ToString() + " products imported");
    }

    protected TraderXProduct CreateProduct(DrJonesProductLine line)
    {
        string qty = line.quantity;
        qty.ToUpper();
        if (qty == "K")
        {
            TraderXMigrationReport.Skipped(SRC, line.className + " (K = vehicle key duplicate service)");
            return null;
        }

        string productId = TraderXProductId.GenerateProductId(line.className);
        if (productId == string.Empty)
            return null;

        TraderXProduct product = TraderXProduct.CreateProduct(line.className, 1.0, -1, MapQuantity(qty), line.buyPrice, line.sellPrice, 0.0, 0, null, null, productId);
        if (!product)
            return null;

        if (qty == "V" || qty == "VNK")
            AttachVehicleParts(product);
        if (qty == "V")
            TraderXMigrationReport.Approx(SRC, line.className + ": sold with a key in Dr Jones (V); TraderX vehicles are locked with TraderXCarLock instead");
        if (qty == "S")
            TraderXMigrationReport.Approx(SRC, line.className + ": steak quantity (S) - TraderX trades it as a full item");

        TraderXExternalMigration.SaveProductIfNew(product);
        m_ProductCount++;
        return product;
    }

    // * / M / W / S / V / VNK = whole item; a number = that fixed quantity.
    protected int MapQuantity(string qty)
    {
        int n = qty.ToInt();
        if (n > 0)
            return TraderXExternalMigration.PackTradeQuantity(0x4, 0x4, Math.Min(n, 4095), Math.Min(n, 8191));
        return TraderXExternalMigration.PackTradeQuantity(0x2, 0x2, 0, 0);
    }

    // Parts fitted at purchase become hidden zero-price part products in the vehicle's default preset
    // (the vehicle price already includes them in Dr Jones). They stay at 0, not -1: preset totals sum part
    // prices without skipping negatives. They are in no category, so the transaction validator refuses them standalone.
    protected void AttachVehicleParts(TraderXProduct vehicle)
    {
        string vehicleKey = vehicle.className;
        vehicleKey.ToLower();
        array<string> parts = m_VehicleParts.Get(vehicleKey);
        if (!parts || parts.Count() == 0)
        {
            TraderXMigrationReport.Approx(SRC, vehicle.className + ": no <VehicleParts> entry - sold without parts");
            return;
        }

        map<string, string> partIds = new map<string, string>();
        array<string> presetParts = new array<string>();
        foreach (string part : parts)
        {
            string cls = part;
            cls.ToLower();
            if (!partIds.Contains(cls))
            {
                string partId = TraderXProductId.GenerateProductId(cls);
                TraderXProduct partProduct = TraderXProduct.CreateProduct(cls, 1.0, -1, TraderXExternalMigration.PackTradeQuantity(0x2, 0x2, 0, 0), 0, -1, 0.0, 0, null, null, partId);
                TraderXExternalMigration.SaveProductIfNew(partProduct);
                partIds.Set(cls, partId);
                if (vehicle.attachments.Find(partId) == -1)
                    vehicle.attachments.Insert(partId);
            }
            presetParts.Insert(partIds.Get(cls));
        }

        if (FileExist(string.Format(TRADERX_PRESETS_FILE, vehicle.productId)))
            return;
        if (!FileExist(TRADERX_PRESETS))
            MakeDirectory(TRADERX_PRESETS);
        TraderXPresets presets = new TraderXPresets();
        presets.productId = vehicle.productId;
        TraderXPreset preset = TraderXPreset.CreateTraderXPreset("Default", vehicle.productId, presetParts);
        TraderXPresetId.AssignPresetIdIfNot(preset);
        presets.presets.Insert(preset);
        presets.defaultPresetId = preset.presetId;
        TraderXPresetRepository.SavePresets(presets);
    }

    protected void SaveCurrency()
    {
        if (!m_Currency || m_Currency.currencies.Count() == 0)
            return;

        TraderXCurrencyTypeCollection collection = new TraderXCurrencyTypeCollection();
        string error;
        if (FileExist(TRADERX_CURRENCY_CONFIG_FILE) && !JsonFileLoader<TraderXCurrencyTypeCollection>.LoadFile(TRADERX_CURRENCY_CONFIG_FILE, collection, error))
        {
            TraderXMigrationReport.Error(SRC, "could not read " + TRADERX_CURRENCY_CONFIG_FILE + " (" + error + ") - currency not imported, file left untouched");
            m_LoadFailed = true;
            return;
        }
        if (!collection.currencyTypes)
            collection.currencyTypes = new array<ref TraderXCurrencyType>();
        foreach (TraderXCurrencyType existing : collection.currencyTypes)
        {
            if (existing.currencyName == m_Currency.currencyName)
            {
                TraderXMigrationReport.Skipped(SRC, "currency type " + m_Currency.currencyName + " already exists - kept");
                return;
            }
        }
        collection.currencyTypes.Insert(m_Currency);
        TraderXCurrencyRepository.Save(collection);
        TraderXMigrationReport.Info(SRC, m_Currency.currencies.Count().ToString() + " notes imported as currency type " + m_Currency.currencyName);
    }

    // ----------------------------------------------------------------------------- NPCs, safezones, parking

    protected void ImportNpcs()
    {
        TraderXGeneralSettings settings = new TraderXGeneralSettings();
        string error;
        if (FileExist(TRADERX_CONFIG_FILE))
        {
            if (!JsonFileLoader<TraderXGeneralSettings>.LoadFile(TRADERX_CONFIG_FILE, settings, error))
            {
                TraderXMigrationReport.Error(SRC, "could not read " + TRADERX_CONFIG_FILE + " (" + error + ") - NPCs and safezones not imported, file left untouched");
                m_LoadFailed = true;
                return;
            }
        }
        else
        {
            settings.version = TRADERX_CURRENT_VERSION;
            settings.serverID = TraderXUuid.GenerateUUID();
        }

        // Read the safezone file before saving anything so a failure leaves both files untouched.
        DrJonesSafeZoneSettings safeZoneSettings = LoadSafeZones();
        if (!safeZoneSettings)
        {
            m_LoadFailed = true;
            return;
        }

        array<ref DrJonesSafeZoneLocation> zones = new array<ref DrJonesSafeZoneLocation>();
        int npcs = 0;
        foreach (DrJonesMarker marker : m_Markers)
        {
            if (marker.traderId < 0 || marker.traderId >= m_Traders.Count())
            {
                TraderXMigrationReport.Skipped(SRC, "marker for unknown trader id " + marker.traderId);
                continue;
            }

            if (marker.safezoneRadius > 0)
            {
                DrJonesSafeZoneLocation zone = new DrJonesSafeZoneLocation();
                zone.name = m_Traders[marker.traderId].name;
                zone.position = marker.position;
                zone.radius = Math.Round(marker.safezoneRadius);
                zone.isEntitiesCleanActive = m_RemoveAnimals;
                zones.Insert(zone);
            }

            if (marker.npcClass == string.Empty)
            {
                TraderXMigrationReport.Skipped(SRC, "marker " + marker.traderId + " has no <Object> (no NPC to place)");
                continue;
            }

            array<ref TraderXLoadoutItem> loadouts = new array<ref TraderXLoadoutItem>();
            foreach (string gear : marker.npcGear)
            {
                loadouts.Insert(new TraderXLoadoutItem(gear));
            }

            array<string> categoryIds = new array<string>();
            categoryIds.Copy(m_CategoryIdsByTrader.Get(marker.traderId));
            array<string> currencies = new array<string>();
            if (m_Currency)
                currencies.Insert(m_Currency.currencyName);

            string traderName = m_Traders[marker.traderId].name;
            TraderXNpc npc = new TraderXNpc(0, marker.npcClass, traderName, traderName, marker.npcPosition, marker.npcOrientation, loadouts, categoryIds, currencies);
            npc.npcId = TraderXExternalMigration.NextFreeNpcId(settings);
            settings.traders.Insert(npc);
            npcs++;

            if (marker.hasVehicleSpawn)
                SaveParking(npc.npcId, marker);
        }

        foreach (TraderXObject decoration : m_Decorations)
        {
            settings.traderObjects.Insert(decoration);
        }

        TraderXSettingsRepository.Save(settings);
        SaveSafeZones(safeZoneSettings, zones);
        TraderXMigrationReport.Info(SRC, string.Format("%1 NPC(s), %2 decoration object(s), %3 safezone(s) imported", npcs, m_Decorations.Count(), zones.Count()));
    }

    protected void SaveParking(int npcId, DrJonesMarker marker)
    {
        if (FileExist(string.Format(TRADERX_VEHICLE_PARKING_FILE, npcId.ToString())))
            return;
        if (!FileExist(TRADERX_VEHICLE_PARKING_DIR))
            MakeDirectory(TRADERX_VEHICLE_PARKING_DIR);

        TraderXVehicleParkingCollection collection = new TraderXVehicleParkingCollection(npcId.ToString());
        collection.positions.Insert(new TraderXVehicleParkingPosition(marker.vehicleSpawn, marker.vehicleSpawnOri, "3 2 6"));
        TraderXVehicleParkingRepository.SaveParkingCollection(collection);
    }

    // Returns null (and reports it) when the safezone file exists but cannot be read.
    protected ref DrJonesSafeZoneSettings LoadSafeZones()
    {
        DrJonesSafeZoneSettings settings = new DrJonesSafeZoneSettings();
        string error;
        if (FileExist(SAFEZONE_FILE))
        {
            if (!JsonFileLoader<DrJonesSafeZoneSettings>.LoadFile(SAFEZONE_FILE, settings, error))
            {
                TraderXMigrationReport.Error(SRC, "could not read " + SAFEZONE_FILE + " (" + error + ") - NPCs and safezones not imported, file left untouched");
                return null;
            }
            if (!settings.locations)
                settings.locations = new array<ref DrJonesSafeZoneLocation>();
            // Drop the placeholder zone TraderXSafeZone writes on a fresh install.
            for (int i = settings.locations.Count() - 1; i >= 0; i--)
            {
                DrJonesSafeZoneLocation existing = settings.locations[i];
                if (existing && existing.name == "Test" && existing.position == vector.Zero)
                    settings.locations.Remove(i);
            }
        }
        return settings;
    }

    protected void SaveSafeZones(DrJonesSafeZoneSettings settings, array<ref DrJonesSafeZoneLocation> zones)
    {
        if (zones.Count() == 0 && m_SafezoneTimeout < 0)
            return;

        int nextId = 1;
        foreach (DrJonesSafeZoneLocation location : settings.locations)
        {
            if (location.id >= nextId)
                nextId = location.id + 1;
        }
        foreach (DrJonesSafeZoneLocation zone : zones)
        {
            zone.id = nextId;
            nextId++;
            settings.locations.Insert(zone);
        }
        if (m_SafezoneTimeout >= 0)
            settings.leaveZoneCountdown = Math.Round(m_SafezoneTimeout);

        array<string> dirs = { "$profile:TraderX/", "$profile:TraderX/TraderXSafeZone/" };
        foreach (string dir : dirs)
        {
            if (!FileExist(dir))
                MakeDirectory(dir);
        }
        string saveError;
        JsonFileLoader<DrJonesSafeZoneSettings>.SaveFile(SAFEZONE_FILE, settings, saveError);
    }

    // ----------------------------------------------------------------------------- helpers

    protected static vector ParseVector(string text)
    {
        TStringArray parts = new TStringArray();
        text.Split(",", parts);
        vector result;
        for (int i = 0; i < 3 && i < parts.Count(); i++)
        {
            string part = parts[i];
            part.TrimInPlace();
            result[i] = part.ToFloat();
        }
        return result;
    }

    protected static bool IsYes(string text)
    {
        string value = text;
        value.TrimInPlace();
        value.ToLower();
        return value == "yes" || value == "true" || value == "1";
    }
}
