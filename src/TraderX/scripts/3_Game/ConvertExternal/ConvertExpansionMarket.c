// DayZ-Expansion-Market -> TraderX converter (one-shot, sentinel ".migrated_expansion", never overwrites).
//
// Sources (Expansion v12/v13 formats):
//   $profile:ExpansionMod/Market/<File>.json        categories (+ IsExchange = currency notes)
//   $profile:ExpansionMod/Traders/<File>.json       trader definitions (categories with buy/sell mode, items, currencies)
//   $profile:ExpansionMod/ATM/<hashedId>.json       player money (imported at player login by TraderXBanking)
//   $mission:expansion/settings/MarketSettings.json global sell percent, deposit limits
//   $mission:expansion/traderzones/*.json           zone price percents and stock
//   $mission:expansion/traders/*.map                NPC placement ("Class.TraderFile|pos|ori|gear,name:X")
// Everything without a TraderX equivalent is written to MigrationReport_Expansion.txt.

class expMarketItem
{
    string ClassName;
    int MaxPriceThreshold;
    int MinPriceThreshold;
    float SellPricePercent = -1;
    int MaxStockThreshold;
    int MinStockThreshold;
    int QuantityPercent = -1;
    ref array<string> SpawnAttachments;
    ref array<string> Variants;
}

class expMarketCategory
{
    int m_Version;
    string DisplayName;
    string Icon;
    int IsExchange;
    float InitStockPercent = 75;
    ref array<ref expMarketItem> Items;
}

class expMarketTrader
{
    string DisplayName;
    string RequiredFaction;
    int MinRequiredReputation;
    ref array<string> Currencies;
    ref array<string> Categories;
    ref map<string, int> Items;
}

class expMarketSettings
{
    float SellPricePercent = 75;
    int MaxDepositMoney;
    int DefaultDepositMoney;
}

class expTraderZone
{
    string m_DisplayName;
    float BuyPricePercent = 100;
    float SellPricePercent = -1;
    ref map<string, int> Stock;
}

class ConvertExpansionMarket
{
    static const string SRC = "Expansion";
    static const string MARKET_DIR = "$profile:ExpansionMod/Market/";
    static const string TRADERS_DIR = "$profile:ExpansionMod/Traders/";
    static const string ATM_DIR = "$profile:ExpansionMod/ATM/";
    static const string MISSION_DIR = "$mission:expansion/";
    static const string ATM_IMPORT_DIR = "$profile:TraderX/TraderXBanking/ExpansionImport/";
    static const string CURRENCY_TYPE = "EXPANSION";

    float m_GlobalSellPercent = 75;
    float m_ZoneSellPercent = -1;
    float m_ZoneBuyPercent = 100;
    ref map<string, int> m_ZoneStock = new map<string, int>();

    ref map<string, ref expMarketCategory> m_Categories = new map<string, ref expMarketCategory>();   // file -> category
    ref map<string, ref expMarketTrader> m_Traders = new map<string, ref expMarketTrader>();          // file -> trader
    ref map<string, ref TraderXProduct> m_ProductByClass = new map<string, ref TraderXProduct>();
    ref map<string, string> m_CategoryIdByFile = new map<string, string>();
    ref map<string, ref TraderXCategory> m_TxCategories = new map<string, ref TraderXCategory>();      // file -> category
    ref TraderXCurrencyType m_Currency;
    ref map<string, string> m_ItemsCategoryByTrader = new map<string, string>();
    // An existing TraderX file could not be read: later steps are skipped and no sentinel is written.
    bool m_LoadFailed;

    static void ConvertIfPresent()
    {
        if (TraderXExternalMigration.IsDone("expansion"))
            return;

        string probe;
        FileAttr probeAttr;
        FindFileHandle probeHandle = TraderXFileSystem.Find(MARKET_DIR + "*.json", probe, probeAttr, FindFileFlags.ALL);
        if (!probeHandle)
            return;
        CloseFindFile(probeHandle);

        ConvertExpansionMarket converter = new ConvertExpansionMarket();
        converter.Run();
    }

    void Run()
    {
        GetTraderXLogger().LogInfo("[TraderX Migration] Expansion Market: importing");
        TraderXExternalMigration.EnsureCatalogDirs();

        LoadSettingsAndZones();
        LoadJsonDir(MARKET_DIR, true);
        LoadJsonDir(TRADERS_DIR, false);

        BuildProducts();
        ApplyTraderModes();
        LinkAttachmentsAndVariants();
        SaveCatalog();
        SaveCurrencies();
        // NPCs are appended, not merged: only import them once the earlier steps succeeded so a retry cannot duplicate them.
        if (!m_LoadFailed)
            ImportTraders();
        StageAtmAccounts();

        if (m_LoadFailed)
        {
            TraderXMigrationReport.Error(SRC, "migration incomplete - not marked done, it will run again on next restart");
            TraderXMigrationReport.Write(SRC);
            return;
        }

        TraderXExternalMigration.MarkDone("expansion", "Expansion Market migrated. categories=" + m_TxCategories.Count() + " products=" + m_ProductByClass.Count());
        TraderXMigrationReport.Write(SRC);
    }

    // ----------------------------------------------------------------------------- loading

    protected void LoadSettingsAndZones()
    {
        expMarketSettings settings = new expMarketSettings();
        string settingsPath = MISSION_DIR + "settings/MarketSettings.json";
        if (!FileExist(settingsPath))
            settingsPath = "$profile:ExpansionMod/Settings/MarketSettings.json";
        string error;
        if (FileExist(settingsPath) && JsonFileLoader<expMarketSettings>.LoadFile(settingsPath, settings, error))
        {
            if (settings.SellPricePercent >= 0)
                m_GlobalSellPercent = settings.SellPricePercent;
            if (settings.MaxDepositMoney > 0)
                TraderXMigrationReport.Approx(SRC, string.Format("ATM limits (MaxDepositMoney %1, DefaultDepositMoney %2): set defaultMaxBalance/defaultStartBalance in TraderXBanking/TraderXBankingConfig.json", settings.MaxDepositMoney, settings.DefaultDepositMoney));
        }

        array<ref expTraderZone> zones = new array<ref expTraderZone>();
        string fileName;
        FileAttr attr;
        FindFileHandle handle = TraderXFileSystem.Find(MISSION_DIR + "traderzones/*.json", fileName, attr, FindFileFlags.ALL);
        if (handle)
        {
            bool hasNext = true;
            while (hasNext)
            {
                expTraderZone zone = new expTraderZone();
                if (JsonFileLoader<expTraderZone>.LoadFile(MISSION_DIR + "traderzones/" + fileName, zone, error))
                    zones.Insert(zone);
                hasNext = FindNextFile(handle, fileName, attr);
            }
            CloseFindFile(handle);
        }

        foreach (expTraderZone z : zones)
        {
            if (!z.Stock)
                continue;
            foreach (string cls, int qty : z.Stock)
            {
                string key = cls;
                key.ToLower();
                if (qty > m_ZoneStock.Get(key))
                    m_ZoneStock.Set(key, qty);
            }
        }

        if (zones.Count() == 1)
        {
            m_ZoneBuyPercent = zones[0].BuyPricePercent;
            m_ZoneSellPercent = zones[0].SellPricePercent;
        }
        else if (zones.Count() > 1)
        {
            TraderXMigrationReport.Approx(SRC, zones.Count().ToString() + " trader zones: TraderX has one price and one stock per product - zone price percents ignored, the highest zone stock is used");
        }
    }

    protected void LoadJsonDir(string dir, bool categories)
    {
        string fileName;
        FileAttr attr;
        FindFileHandle handle = TraderXFileSystem.Find(dir + "*.json", fileName, attr, FindFileFlags.ALL);
        if (!handle)
            return;

        bool hasNext = true;
        while (hasNext)
        {
            string baseName = fileName;
            baseName.Replace(".json", "");
            string error;
            if (categories)
            {
                expMarketCategory category = new expMarketCategory();
                if (JsonFileLoader<expMarketCategory>.LoadFile(dir + fileName, category, error) && category.Items)
                    m_Categories.Set(baseName, category);
                else
                    TraderXMigrationReport.Skipped(SRC, "unreadable market file " + fileName + " " + error);
            }
            else
            {
                expMarketTrader trader = new expMarketTrader();
                if (JsonFileLoader<expMarketTrader>.LoadFile(dir + fileName, trader, error))
                    m_Traders.Set(baseName, trader);
                else
                    TraderXMigrationReport.Skipped(SRC, "unreadable trader file " + fileName + " " + error);
            }
            hasNext = FindNextFile(handle, fileName, attr);
        }
        CloseFindFile(handle);
    }

    // ----------------------------------------------------------------------------- products

    protected void BuildProducts()
    {
        m_Currency = new TraderXCurrencyType(CURRENCY_TYPE);

        foreach (string file, expMarketCategory category : m_Categories)
        {
            if (category.IsExchange)
            {
                foreach (expMarketItem note : category.Items)
                {
                    m_Currency.AddCurrency(note.ClassName, note.MinPriceThreshold);
                }
                continue;
            }

            string name = ReadableName(category.DisplayName, file);
            string categoryId = TraderXCategoryId.GenerateCategoryId(name);
            TraderXCategory txCategory = TraderXCategory.CreateCategory(name, 0, categoryId);
            m_CategoryIdByFile.Set(file, categoryId);
            m_TxCategories.Set(file, txCategory);

            foreach (expMarketItem item : category.Items)
            {
                TraderXProduct product = GetOrCreateProduct(item, category.InitStockPercent);
                if (product)
                    txCategory.AddProduct(product);
            }
        }
    }

    protected TraderXProduct GetOrCreateProduct(expMarketItem item, float initStockPercent)
    {
        string cls = item.ClassName;
        cls.ToLower();
        if (m_ProductByClass.Contains(cls))
            return m_ProductByClass.Get(cls);

        string productId = TraderXProductId.GenerateProductId(cls);
        if (productId == string.Empty)
            return null;

        float buyRatio = m_ZoneBuyPercent / 100.0;
        int buyPrice = Math.Round(item.MaxPriceThreshold * buyRatio);
        int minPrice = Math.Round(item.MinPriceThreshold * buyRatio);

        // Expansion: equal min/max stock thresholds = static (unlimited) stock.
        int maxStock = -1;
        float coefficient = 1.0;
        if (item.MaxStockThreshold > 0 && item.MinStockThreshold != item.MaxStockThreshold)
        {
            maxStock = item.MaxStockThreshold;
            // Price curve between Max (empty) and Min (full stock): same end points, TraderX curve shape.
            if (minPrice > 0 && minPrice < buyPrice && maxStock > 1)
            {
                float buyAsFloat = buyPrice;
                coefficient = Math.Pow(minPrice / buyAsFloat, 1.0 / (maxStock - 1));
            }
        }

        float sellPercent = item.SellPricePercent;
        if (sellPercent < 0)
            sellPercent = m_ZoneSellPercent;
        if (sellPercent < 0)
            sellPercent = m_GlobalSellPercent;
        int sellPrice = Math.Round(buyPrice * sellPercent / 100.0);

        TraderXProduct product = TraderXProduct.CreateProduct(cls, coefficient, maxStock, QuantityToTrade(item.QuantityPercent), buyPrice, sellPrice, 0.0, 0, null, null, productId);
        if (!product)
            return null;

        m_ProductByClass.Set(cls, product);
        SeedStock(product, initStockPercent);
        return product;
    }

    // Expansion QuantityPercent: -1/100 = full, 0 = empty, 1..99 = percentage. Selling mirrors buying
    // (TraderX does not scale the sell price with the remaining quantity).
    protected int QuantityToTrade(int quantityPercent)
    {
        if (quantityPercent < 0 || quantityPercent >= 100)
            return TraderXExternalMigration.PackTradeQuantity(0x2, 0x2, 0, 0);
        if (quantityPercent == 0)
            return TraderXExternalMigration.PackTradeQuantity(0x0, 0x1, 0, 0);
        return TraderXExternalMigration.PackTradeQuantity(0x3, 0x3, quantityPercent, quantityPercent);
    }

    protected void SeedStock(TraderXProduct product, float initStockPercent)
    {
        if (product.maxStock < 0)
            return;
        if (FileExist(string.Format(TRADERX_STOCK_FILE, product.productId)))
            return;

        int stock = Math.Round(product.maxStock * initStockPercent / 100.0);
        if (m_ZoneStock.Contains(product.className))
            stock = m_ZoneStock.Get(product.className);
        if (stock > product.maxStock)
            stock = product.maxStock;

        if (!FileExist(TRADERX_STOCK_DIR))
            MakeDirectory(TRADERX_STOCK_DIR);
        TraderXProductStockRepository.Save(new TraderXProductStock(product.productId, stock));
    }

    // Trader category modes: 0 = buy only, 1 = buy and sell, 2 = sell only.
    protected void ApplyTraderModes()
    {
        map<string, int> buyable = new map<string, int>();
        map<string, int> sellable = new map<string, int>();
        foreach (string traderFile, expMarketTrader trader : m_Traders)
        {
            if (!trader.Categories)
                continue;
            foreach (string entry : trader.Categories)
            {
                string file = CategoryFile(entry);
                int mode = CategoryMode(entry);
                if (mode != 2)
                    buyable.Set(file, 1);
                if (mode != 0)
                    sellable.Set(file, 1);
            }
        }

        foreach (string catFile, TraderXCategory category : m_TxCategories)
        {
            if (!buyable.Contains(catFile) && !sellable.Contains(catFile))
                continue;
            foreach (string productId : category.productIds)
            {
                TraderXProduct product = FindProductById(productId);
                if (!product)
                    continue;
                if (!sellable.Contains(catFile))
                    product.sellPrice = -1;
                if (!buyable.Contains(catFile))
                    product.buyPrice = -1;
            }
        }
    }

    // Attachments (product ids) + a default preset; variants become products outside the category.
    protected void LinkAttachmentsAndVariants()
    {
        foreach (string file, expMarketCategory category : m_Categories)
        {
            if (category.IsExchange)
                continue;
            foreach (expMarketItem item : category.Items)
            {
                string cls = item.ClassName;
                cls.ToLower();
                TraderXProduct product = m_ProductByClass.Get(cls);
                if (!product)
                    continue;

                array<string> attachmentIds = ResolveClasses(item.SpawnAttachments, "attachment of " + cls);
                if (attachmentIds.Count() > 0)
                {
                    product.attachments.Copy(attachmentIds);
                    SaveDefaultPreset(product, attachmentIds);
                }

                if (!item.Variants)
                    continue;
                foreach (string variant : item.Variants)
                {
                    string variantClass = variant;
                    variantClass.ToLower();
                    TraderXProduct variantProduct = m_ProductByClass.Get(variantClass);
                    if (!variantProduct)
                    {
                        variantProduct = CloneAsVariant(product, variantClass);
                        if (!variantProduct)
                            continue;
                    }
                    if (product.variants.Find(variantProduct.productId) == -1)
                        product.variants.Insert(variantProduct.productId);
                }
            }
        }
    }

    protected array<string> ResolveClasses(array<string> classNames, string context)
    {
        array<string> ids = new array<string>();
        if (!classNames)
            return ids;
        foreach (string className : classNames)
        {
            string cls = className;
            cls.ToLower();
            TraderXProduct product = m_ProductByClass.Get(cls);
            if (product)
                ids.Insert(product.productId);
            else
                TraderXMigrationReport.Skipped(SRC, cls + " (" + context + ") is not a market item, so TraderX cannot sell it as an attachment");
        }
        return ids;
    }

    protected TraderXProduct CloneAsVariant(TraderXProduct parent, string variantClass)
    {
        string productId = TraderXProductId.GenerateProductId(variantClass);
        if (productId == string.Empty)
            return null;
        TraderXProduct variant = TraderXProduct.CreateProduct(variantClass, parent.coefficient, parent.maxStock, parent.tradeQuantity, parent.buyPrice, parent.sellPrice, 0.0, 0, null, null, productId);
        if (!variant)
            return null;
        variant.attachments.Copy(parent.attachments);
        m_ProductByClass.Set(variantClass, variant);
        return variant;
    }

    protected void SaveDefaultPreset(TraderXProduct product, array<string> attachmentIds)
    {
        if (FileExist(string.Format(TRADERX_PRESETS_FILE, product.productId)))
            return;
        if (!FileExist(TRADERX_PRESETS))
            MakeDirectory(TRADERX_PRESETS);

        TraderXPresets presets = new TraderXPresets();
        presets.productId = product.productId;
        TraderXPreset preset = TraderXPreset.CreateTraderXPreset("Default", product.productId, attachmentIds);
        TraderXPresetId.AssignPresetIdIfNot(preset);
        presets.presets.Insert(preset);
        presets.defaultPresetId = preset.presetId;
        TraderXPresetRepository.SavePresets(presets);
    }

    protected void SaveCatalog()
    {
        foreach (string cls, TraderXProduct product : m_ProductByClass)
        {
            TraderXExternalMigration.SaveProductIfNew(product);
        }
        foreach (string file, TraderXCategory category : m_TxCategories)
        {
            TraderXExternalMigration.SaveCategoryIfNew(category);
        }
        TraderXMigrationReport.Info(SRC, m_TxCategories.Count().ToString() + " categories and " + m_ProductByClass.Count().ToString() + " products imported");
        TraderXMigrationReport.Approx(SRC, "dynamic prices: TraderX keeps Expansion's highest and lowest price, the curve between them differs");
        TraderXMigrationReport.Approx(SRC, "selling a partly used item: Expansion scales the price, TraderX buys the quantity defined for the product");
    }

    protected void SaveCurrencies()
    {
        if (m_Currency.currencies.Count() == 0)
            return;

        TraderXCurrencyTypeCollection collection = new TraderXCurrencyTypeCollection();
        string error;
        if (FileExist(TRADERX_CURRENCY_CONFIG_FILE) && !JsonFileLoader<TraderXCurrencyTypeCollection>.LoadFile(TRADERX_CURRENCY_CONFIG_FILE, collection, error))
        {
            TraderXMigrationReport.Error(SRC, "could not read " + TRADERX_CURRENCY_CONFIG_FILE + " (" + error + ") - currencies not imported, file left untouched");
            m_LoadFailed = true;
            return;
        }
        if (!collection.currencyTypes)
            collection.currencyTypes = new array<ref TraderXCurrencyType>();

        foreach (TraderXCurrencyType existing : collection.currencyTypes)
        {
            if (existing.currencyName == CURRENCY_TYPE)
            {
                TraderXMigrationReport.Skipped(SRC, "currency type EXPANSION already exists - kept");
                return;
            }
        }
        collection.currencyTypes.Insert(m_Currency);
        TraderXCurrencyRepository.Save(collection);
        TraderXMigrationReport.Info(SRC, m_Currency.currencies.Count().ToString() + " exchange notes imported as currency type EXPANSION");
    }

    // ----------------------------------------------------------------------------- traders / NPCs

    protected void ImportTraders()
    {
        TraderXGeneralSettings settings = new TraderXGeneralSettings();
        string error;
        if (FileExist(TRADERX_CONFIG_FILE))
        {
            if (!JsonFileLoader<TraderXGeneralSettings>.LoadFile(TRADERX_CONFIG_FILE, settings, error))
            {
                TraderXMigrationReport.Error(SRC, "could not read " + TRADERX_CONFIG_FILE + " (" + error + ") - trader NPCs not imported, file left untouched");
                m_LoadFailed = true;
                return;
            }
        }
        else
        {
            settings.version = TRADERX_CURRENT_VERSION;
            settings.serverID = TraderXUuid.GenerateUUID();
        }

        int npcs = 0;
        string fileName;
        FileAttr attr;
        FindFileHandle handle = TraderXFileSystem.Find(MISSION_DIR + "traders/*.map", fileName, attr, FindFileFlags.ALL);
        if (handle)
        {
            bool hasNext = true;
            while (hasNext)
            {
                npcs += ImportMapFile(MISSION_DIR + "traders/" + fileName, settings);
                hasNext = FindNextFile(handle, fileName, attr);
            }
            CloseFindFile(handle);
        }

        foreach (string traderFile, expMarketTrader trader : m_Traders)
        {
            if (trader.RequiredFaction != string.Empty || trader.MinRequiredReputation > 0)
                TraderXMigrationReport.Skipped(SRC, "trader " + traderFile + ": faction/reputation requirements");
        }

        TraderXSettingsRepository.Save(settings);
        TraderXMigrationReport.Info(SRC, npcs.ToString() + " trader NPC(s) imported from expansion/traders/*.map");
    }

    protected int ImportMapFile(string path, TraderXGeneralSettings settings)
    {
        FileHandle fh = OpenFile(path, FileMode.READ);
        if (!fh)
            return 0;

        int count = 0;
        string line;
        while (FGets(fh, line) >= 0)
        {
            line.TrimInPlace();
            if (line == string.Empty || line.IndexOf("//") == 0)
                continue;
            if (ImportMapLine(line, settings))
                count++;
        }
        CloseFile(fh);
        return count;
    }

    // "ExpansionTraderDenis.Weapons|x y z[,waypoints]|ori|gear,gear+attachment,name:Denis"
    protected bool ImportMapLine(string line, TraderXGeneralSettings settings)
    {
        TStringArray parts = new TStringArray();
        line.Split("|", parts);
        if (parts.Count() < 3)
            return false;

        int dot = parts[0].IndexOf(".");
        if (dot < 0)
            return false;
        string expansionClass = parts[0].Substring(0, dot);
        string traderFile = parts[0].Substring(dot + 1, parts[0].Length() - dot - 1);

        string positionText = parts[1];
        int comma = positionText.IndexOf(",");
        if (comma > 0)
        {
            positionText = positionText.Substring(0, comma);
            TraderXMigrationReport.Skipped(SRC, "AI waypoints of " + parts[0]);
        }

        string givenName = traderFile;
        array<ref TraderXLoadoutItem> loadouts = new array<ref TraderXLoadoutItem>();
        if (parts.Count() > 3)
            ParseGear(parts[3], loadouts, givenName);

        array<string> categoriesId = new array<string>();
        array<string> currencies = new array<string>();
        expMarketTrader trader = m_Traders.Get(traderFile);
        string role = traderFile;
        if (trader)
        {
            role = ReadableName(trader.DisplayName, traderFile);
            CollectTraderCategories(trader, traderFile, categoriesId);
            if (trader.Currencies && trader.Currencies.Count() > 0)
                currencies.Insert(CURRENCY_TYPE);
        }
        else
        {
            TraderXMigrationReport.Skipped(SRC, "NPC " + parts[0] + " refers to missing trader file " + traderFile + ".json");
        }

        TraderXNpc npc = new TraderXNpc(0, NpcClass(expansionClass), givenName, role, positionText.ToVector(), parts[2].ToVector(), loadouts, categoriesId, currencies);
        npc.npcId = TraderXExternalMigration.NextFreeNpcId(settings);
        settings.traders.Insert(npc);
        return true;
    }

    protected void ParseGear(string gearText, array<ref TraderXLoadoutItem> loadouts, inout string givenName)
    {
        TStringArray gear = new TStringArray();
        gearText.Split(",", gear);
        foreach (string piece : gear)
        {
            piece.TrimInPlace();
            if (piece == string.Empty)
                continue;
            if (piece.IndexOf("name:") == 0)
            {
                givenName = piece.Substring(5, piece.Length() - 5);
                continue;
            }
            if (piece.IndexOf("loadout:") == 0 || piece.IndexOf("faction:") == 0)
            {
                TraderXMigrationReport.Skipped(SRC, "NPC " + piece);
                continue;
            }

            TStringArray chain = new TStringArray();
            piece.Split("+", chain);
            TraderXLoadoutItem loadoutItem = new TraderXLoadoutItem(chain[0]);
            for (int i = 1; i < chain.Count(); i++)
            {
                loadoutItem.attachments.Insert(new TraderXLoadoutAttachment(chain[i], -1));
            }
            loadouts.Insert(loadoutItem);
        }
    }

    protected void CollectTraderCategories(expMarketTrader trader, string traderFile, array<string> categoriesId)
    {
        if (trader.Categories)
        {
            foreach (string entry : trader.Categories)
            {
                string file = CategoryFile(entry);
                if (m_CategoryIdByFile.Contains(file))
                    categoriesId.Insert(m_CategoryIdByFile.Get(file));
                else if (!m_Categories.Contains(file) || !m_Categories.Get(file).IsExchange)
                    TraderXMigrationReport.Skipped(SRC, "trader " + traderFile + " lists missing category " + file);
            }
        }

        if (!trader.Items || trader.Items.Count() == 0)
            return;

        if (m_ItemsCategoryByTrader.Contains(traderFile))
        {
            string cachedId = m_ItemsCategoryByTrader.Get(traderFile);
            if (cachedId != string.Empty)
                categoriesId.Insert(cachedId);
            return;
        }
        m_ItemsCategoryByTrader.Set(traderFile, string.Empty);

        // Single items sold by this trader: their own category (one per trader file).
        string name = ReadableName(trader.DisplayName, traderFile) + " - items";
        string categoryId = TraderXCategoryId.GenerateCategoryId(name);
        TraderXCategory category = TraderXCategory.CreateCategory(name, 0, categoryId);
        foreach (string cls, int mode : trader.Items)
        {
            string lower = cls;
            lower.ToLower();
            if (mode == 3)
            {
                TraderXMigrationReport.Skipped(SRC, "trader " + traderFile + " item " + lower + " (attachment-only listing)");
                continue;
            }
            TraderXProduct product = m_ProductByClass.Get(lower);
            if (product)
                category.AddProduct(product);
            else
                TraderXMigrationReport.Skipped(SRC, "trader " + traderFile + " item " + lower + " is not a market item");
        }
        if (category.productIds.Count() > 0 && TraderXExternalMigration.SaveCategoryIfNew(category))
        {
            categoriesId.Insert(categoryId);
            m_ItemsCategoryByTrader.Set(traderFile, categoryId);
        }
    }

    // Expansion NPC classes exist only with Expansion loaded: use the matching vanilla survivor.
    protected string NpcClass(string expansionClass)
    {
        if (GetGame().ConfigIsExisting("CfgVehicles " + expansionClass))
            return expansionClass;

        string name = expansionClass;
        name.Replace("ExpansionTrader", "");
        array<string> candidates = { "SurvivorM_" + name, "SurvivorF_" + name };
        foreach (string candidate : candidates)
        {
            if (GetGame().ConfigIsExisting("CfgVehicles " + candidate))
                return candidate;
        }

        TraderXMigrationReport.Approx(SRC, "NPC class " + expansionClass + " has no vanilla equivalent - using SurvivorM_Mirek");
        return "SurvivorM_Mirek";
    }

    // ----------------------------------------------------------------------------- ATM

    // Expansion keys ATM files by identity.GetId() (a one-way hash): TraderXBanking credits them when
    // the player logs in (TXBExpansionAtmImport).
    protected void StageAtmAccounts()
    {
        string fileName;
        FileAttr attr;
        FindFileHandle handle = TraderXFileSystem.Find(ATM_DIR + "*.json", fileName, attr, FindFileFlags.ALL);
        if (!handle)
            return;

        array<string> dirs = { "$profile:TraderX/", "$profile:TraderX/TraderXBanking/", ATM_IMPORT_DIR };
        foreach (string dir : dirs)
        {
            if (!FileExist(dir))
                MakeDirectory(dir);
        }

        int staged = 0;
        bool hasNext = true;
        while (hasNext)
        {
            if (!FileExist(ATM_IMPORT_DIR + fileName))
            {
                CopyFile(ATM_DIR + fileName, ATM_IMPORT_DIR + fileName);
                staged++;
            }
            hasNext = FindNextFile(handle, fileName, attr);
        }
        CloseFindFile(handle);
        TraderXMigrationReport.Info(SRC, staged.ToString() + " ATM account(s) staged - credited to the TraderX bank when each player next logs in");
    }

    // ----------------------------------------------------------------------------- helpers

    protected static string CategoryFile(string entry)
    {
        int colon = entry.IndexOf(":");
        if (colon < 0)
            return entry;
        return entry.Substring(0, colon);
    }

    protected static int CategoryMode(string entry)
    {
        int colon = entry.IndexOf(":");
        if (colon < 0)
            return 1;
        return entry.Substring(colon + 1, entry.Length() - colon - 1).ToInt();
    }

    // "#STR_..." keys are client localisation keys: show the file name instead.
    protected static string ReadableName(string displayName, string fileName)
    {
        if (displayName != string.Empty && displayName.IndexOf("#") != 0)
            return displayName;
        string name = fileName;
        name.Replace("_", " ");
        return name;
    }

    protected TraderXProduct FindProductById(string productId)
    {
        foreach (string cls, TraderXProduct product : m_ProductByClass)
        {
            if (product.productId == productId)
                return product;
        }
        return null;
    }
}
