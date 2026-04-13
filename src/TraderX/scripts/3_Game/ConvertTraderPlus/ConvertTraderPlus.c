// ============================================================================
// TraderPlus V1 → TraderX Full Converter
// Server-side startup task that detects and migrates TraderPlus configs
// ============================================================================

class ConvertTraderPlus
{
    // Lookup: V1 traderId → list of category names assigned to that trader
    ref map<int, ref array<string>> m_TraderCategoryMap;
    // Lookup: V1 traderId → licenses required
    ref map<int, ref array<string>> m_TraderLicenseMap;
    // Lookup: V1 traderId → currencies accepted
    ref map<int, ref array<string>> m_TraderCurrencyMap;
    // Lookup: category name → generated TraderX categoryId
    ref map<string, string> m_CategoryIdMap;

    ref TPV1GeneralConfig m_GeneralConfig;
    ref TPV1PriceConfig m_PriceConfig;
    ref TPV1IDsConfig m_IDsConfig;

    int m_ProductCount;
    int m_CategoryCount;
    int m_TraderCount;

    // ========================================================================
    // Entry point — called from TraderXModule.InitializeModule()
    // ========================================================================
    static void TryConvert()
    {
        if (!GetGame().IsServer())
            return;

        if (!ShouldConvert())
            return;

        GetTraderXLogger().LogInfo("============================================================");
        GetTraderXLogger().LogInfo("[TPV1 Converter] TraderPlus V1 configs detected — starting conversion");
        GetTraderXLogger().LogInfo("============================================================");

        ConvertTraderPlus converter = new ConvertTraderPlus();
        converter.Run();
    }

    static bool ShouldConvert()
    {
        // Only convert if the V1 general config exists AND TraderX general settings do NOT
        if (!FileExist(TPV1_GENERAL_CONFIG))
            return false;

        if (FileExist(TRADERX_CONFIG_FILE))
        {
            GetTraderXLogger().LogInfo("[TPV1 Converter] Skipping — TraderX config already exists at: " + TRADERX_CONFIG_FILE);
            return false;
        }

        return true;
    }

    // ========================================================================
    // Main conversion pipeline
    // ========================================================================
    void Run()
    {
        m_TraderCategoryMap = new map<int, ref array<string>>();
        m_TraderLicenseMap = new map<int, ref array<string>>();
        m_TraderCurrencyMap = new map<int, ref array<string>>();
        m_CategoryIdMap = new map<string, string>();
        m_ProductCount = 0;
        m_CategoryCount = 0;
        m_TraderCount = 0;

        EnsureDirectories();

        // Phase 1: Load all V1 configs
        if (!LoadV1Configs())
            return;

        // Phase 2: Convert IDs config (trader → category/license/currency mappings)
        ConvertIDsConfig();

        // Phase 3: Convert price config (categories + products)
        ConvertPriceConfig();

        // Phase 4: Convert general config (settings, traders, currencies, objects)
        ConvertGeneralConfig();

        // Phase 5: Convert SafeZone config
        ConvertSafeZoneConfig();

        // Phase 6: Convert insurance and receipt settings from V1 flags
        ConvertTraderPlusInsurance.Convert(m_GeneralConfig);

        // Phase 7: Migrate player bank accounts
        MigrateBankAccounts();

        // Phase 8: Backup V1 files
        BackupV1Files();

        GetTraderXLogger().LogInfo("============================================================");
        GetTraderXLogger().LogInfo("[TPV1 Converter] Conversion complete!");
        GetTraderXLogger().LogInfo(string.Format("  Categories: %1", m_CategoryCount));
        GetTraderXLogger().LogInfo(string.Format("  Products:   %1", m_ProductCount));
        GetTraderXLogger().LogInfo(string.Format("  Traders:    %1", m_TraderCount));
        GetTraderXLogger().LogInfo("============================================================");
    }

    // ========================================================================
    // Phase 1: Load V1 configs
    // ========================================================================
    private bool LoadV1Configs()
    {
        // General config (required)
        m_GeneralConfig = new TPV1GeneralConfig();
        TraderXJsonLoader<TPV1GeneralConfig>.LoadFromFile(TPV1_GENERAL_CONFIG, m_GeneralConfig);
        if (!m_GeneralConfig)
        {
            GetTraderXLogger().LogError("[TPV1 Converter] Failed to load general config");
            return false;
        }
        GetTraderXLogger().LogInfo("[TPV1 Converter] Loaded general config (version: " + m_GeneralConfig.Version + ")");

        // Price config (required)
        m_PriceConfig = new TPV1PriceConfig();
        if (FileExist(TPV1_PRICE_CONFIG))
        {
            TraderXJsonLoader<TPV1PriceConfig>.LoadFromFile(TPV1_PRICE_CONFIG, m_PriceConfig);
            GetTraderXLogger().LogInfo(string.Format("[TPV1 Converter] Loaded price config with %1 categories", m_PriceConfig.TraderCategories.Count()));
        }
        else
        {
            GetTraderXLogger().LogWarning("[TPV1 Converter] Price config not found — no products will be imported");
        }

        // IDs config (optional but important)
        m_IDsConfig = new TPV1IDsConfig();
        if (FileExist(TPV1_IDS_CONFIG))
        {
            TraderXJsonLoader<TPV1IDsConfig>.LoadFromFile(TPV1_IDS_CONFIG, m_IDsConfig);
            GetTraderXLogger().LogInfo(string.Format("[TPV1 Converter] Loaded IDs config with %1 trader assignments", m_IDsConfig.IDs.Count()));
        }

        return true;
    }

    // ========================================================================
    // Phase 2: Build trader → category/license/currency lookup maps from IDsConfig
    // ========================================================================
    private void ConvertIDsConfig()
    {
        if (!m_IDsConfig || !m_IDsConfig.IDs)
            return;

        foreach (TPV1TraderIds ids : m_IDsConfig.IDs)
        {
            m_TraderCategoryMap.Set(ids.Id, ids.Categories);
            m_TraderLicenseMap.Set(ids.Id, ids.LicencesRequired);
            m_TraderCurrencyMap.Set(ids.Id, ids.CurrenciesAccepted);
        }

        GetTraderXLogger().LogInfo(string.Format("[TPV1 Converter] Built lookup maps for %1 traders", m_IDsConfig.IDs.Count()));
    }

    // ========================================================================
    // Phase 3: Convert categories and products
    // ========================================================================
    private void ConvertPriceConfig()
    {
        if (!m_PriceConfig || !m_PriceConfig.TraderCategories)
            return;

        // Build a set of licenses required per category by scanning all trader assignments
        // V1 puts licenses on the NPC-category link; TraderX puts them on the category itself
        map<string, ref array<string>> categoryLicenses = new map<string, ref array<string>>();
        BuildCategoryLicenseMap(categoryLicenses);

        foreach (TPV1Category v1Cat : m_PriceConfig.TraderCategories)
        {
            if (!v1Cat || v1Cat.CategoryName == string.Empty)
                continue;

            string categoryId = TraderXCategoryId.GenerateCategoryId(v1Cat.CategoryName);
            if (categoryId == string.Empty)
            {
                GetTraderXLogger().LogError("[TPV1 Converter] Failed to generate ID for category: " + v1Cat.CategoryName);
                continue;
            }

            TraderXCategory txCategory = TraderXCategory.CreateCategory(v1Cat.CategoryName, ETraderXCategoryType.NONE, categoryId);

            // Apply licenses from the NPC-level mapping
            if (categoryLicenses.Contains(v1Cat.CategoryName))
            {
                array<string> licenses = categoryLicenses.Get(v1Cat.CategoryName);
                foreach (string lic : licenses)
                {
                    if (txCategory.licensesRequired.Find(lic) == -1)
                        txCategory.licensesRequired.Insert(lic);
                }
            }

            // Convert each product CSV string
            int catProductCount = 0;
            foreach (string productCsv : v1Cat.Products)
            {
                if (productCsv == string.Empty)
                    continue;

                TraderXProduct txProduct = TraderPlusProductParser.Parse(productCsv);
                if (txProduct)
                {
                    TraderXProductRepository.Save(txProduct);
                    txCategory.AddProduct(txProduct);
                    catProductCount++;
                    m_ProductCount++;
                }
                else
                {
                    GetTraderXLogger().LogWarning("[TPV1 Converter] Failed to parse product: " + productCsv);
                }
            }

            TraderXCategoryRepository.Save(txCategory);
            m_CategoryIdMap.Set(v1Cat.CategoryName, categoryId);
            m_CategoryCount++;

            GetTraderXLogger().LogDebug(string.Format("[TPV1 Converter] Category '%1' → %2 products", v1Cat.CategoryName, catProductCount));
        }
    }

    // V1 puts license requirements on the trader-category link (IDsConfig).
    // TraderX puts them on the category itself. We merge: if ANY trader requires
    // a license for a category, that category gets those licenses.
    private void BuildCategoryLicenseMap(map<string, ref array<string>> outMap)
    {
        foreach (int traderId, array<string> licenses : m_TraderLicenseMap)
        {
            if (!licenses || licenses.Count() == 0)
                continue;

            array<string> categories;
            if (!m_TraderCategoryMap.Find(traderId, categories))
                continue;

            foreach (string catName : categories)
            {
                if (!outMap.Contains(catName))
                    outMap.Set(catName, new array<string>());

                foreach (string lic : licenses)
                {
                    if (outMap.Get(catName).Find(lic) == -1)
                        outMap.Get(catName).Insert(lic);
                }
            }
        }
    }

    // ========================================================================
    // Phase 4: Convert general config (settings, traders, currencies)
    // ========================================================================
    private void ConvertGeneralConfig()
    {
        TraderXGeneralSettings txSettings = new TraderXGeneralSettings();
        txSettings.version = TRADERX_CURRENT_VERSION;
        txSettings.GenerateServerID(txSettings.serverID);
        txSettings.paymentPriority = "cash_first";

        // Licenses
        if (m_GeneralConfig.Licences)
        {
            foreach (string lic : m_GeneralConfig.Licences)
            {
                txSettings.licenses.Insert(new TraderXLicense(lic, string.Empty));
            }
        }

        // Accepted states
        if (m_GeneralConfig.AcceptedStates)
        {
            txSettings.acceptedStates = new TraderXStates(
                m_GeneralConfig.AcceptedStates.AcceptWorn,
                m_GeneralConfig.AcceptedStates.AcceptDamaged,
                m_GeneralConfig.AcceptedStates.AcceptBadlyDamaged
            );
        }
        else
        {
            txSettings.acceptedStates = new TraderXStates(true, true, true);
        }

        // Trader objects
        if (m_GeneralConfig.TraderObjects)
        {
            foreach (TPV1TraderObject v1Obj : m_GeneralConfig.TraderObjects)
            {
                txSettings.traderObjects.Insert(new TraderXObject(v1Obj.ObjectName, v1Obj.Position, v1Obj.Orientation));
            }
            GetTraderXLogger().LogInfo(string.Format("[TPV1 Converter] Converted %1 trader objects", m_GeneralConfig.TraderObjects.Count()));
        }

        // Currencies
        ConvertCurrencies();

        // Traders (NPCs)
        ConvertTraders(txSettings);

        TraderXSettingsRepository.Save(txSettings);
        GetTraderXLogger().LogInfo("[TPV1 Converter] General settings saved");
    }

    private void ConvertCurrencies()
    {
        if (!m_GeneralConfig.Currencies || m_GeneralConfig.Currencies.Count() == 0)
        {
            GetTraderXLogger().LogInfo("[TPV1 Converter] No currencies to convert");
            return;
        }

        // Group currency items by detected type (USD, EUR, RUB, etc.)
        map<string, ref TraderXCurrencyType> currencyTypes = new map<string, ref TraderXCurrencyType>();

        foreach (TPV1Currency v1Cur : m_GeneralConfig.Currencies)
        {
            // V1 allows comma-separated classnames per denomination
            TStringArray classNames = new TStringArray();
            v1Cur.ClassName.Split(",", classNames);

            foreach (string rawClassName : classNames)
            {
                string className = rawClassName;
                className.Trim();
                if (className == string.Empty)
                    continue;

                string typeName = DetectCurrencyType(className);

                if (!currencyTypes.Contains(typeName))
                    currencyTypes.Set(typeName, new TraderXCurrencyType(typeName));

                currencyTypes.Get(typeName).AddCurrency(className, v1Cur.Value);
            }
        }

        TraderXCurrencyTypeCollection collection = new TraderXCurrencyTypeCollection();
        foreach (string typeName, TraderXCurrencyType curType : currencyTypes)
        {
            collection.currencyTypes.Insert(curType);
        }

        TraderXCurrencyRepository.Save(collection);
        GetTraderXLogger().LogInfo(string.Format("[TPV1 Converter] Converted %1 currency types", currencyTypes.Count()));
    }

    private string DetectCurrencyType(string className)
    {
        string lower = className;
        lower.ToLower();

        if (lower.Contains("dollar") || lower.Contains("usd"))
            return "USD";
        if (lower.Contains("euro") || lower.Contains("eur"))
            return "EUR";
        if (lower.Contains("ruble") || lower.Contains("rub"))
            return "RUB";
        if (lower.Contains("pound") || lower.Contains("gbp"))
            return "GBP";
        if (lower.Contains("yen") || lower.Contains("jpy"))
            return "JPY";

        return "DEFAULT";
    }

    private void ConvertTraders(TraderXGeneralSettings txSettings)
    {
        if (!m_GeneralConfig.Traders)
            return;

        foreach (TPV1TraderData v1Trader : m_GeneralConfig.Traders)
        {
            // Loadouts
            ref array<ref TraderXLoadoutItem> loadouts = new array<ref TraderXLoadoutItem>();
            if (v1Trader.Clothes)
            {
                foreach (string cloth : v1Trader.Clothes)
                {
                    loadouts.Insert(new TraderXLoadoutItem(cloth));
                }
            }

            // Map category names → TraderX category IDs
            ref array<string> categoryIds = new array<string>();
            if (m_TraderCategoryMap.Contains(v1Trader.Id))
            {
                array<string> catNames = m_TraderCategoryMap.Get(v1Trader.Id);
                foreach (string catName : catNames)
                {
                    string catId;
                    if (m_CategoryIdMap.Find(catName, catId))
                        categoryIds.Insert(catId);
                    else
                        GetTraderXLogger().LogWarning("[TPV1 Converter] Category '" + catName + "' not found for trader ID " + v1Trader.Id);
                }
            }

            // Currencies accepted
            ref array<string> currencies = new array<string>();
            if (m_TraderCurrencyMap.Contains(v1Trader.Id))
                currencies = m_TraderCurrencyMap.Get(v1Trader.Id);

            TraderXNpc txNpc = new TraderXNpc(
                v1Trader.Id,
                v1Trader.Name,
                v1Trader.GivenName,
                v1Trader.Role,
                v1Trader.Position,
                v1Trader.Orientation,
                loadouts,
                categoryIds,
                currencies
            );

            txSettings.traders.Insert(txNpc);
            m_TraderCount++;

            GetTraderXLogger().LogDebug(string.Format("[TPV1 Converter] Trader '%1' (%2) → %3 categories", v1Trader.GivenName, v1Trader.Role, categoryIds.Count()));
        }
    }

    // ========================================================================
    // Phase 5: Convert SafeZone config
    // ========================================================================
    private void ConvertSafeZoneConfig()
    {
        if (!FileExist(TPV1_SAFEZONE_CONFIG))
        {
            GetTraderXLogger().LogInfo("[TPV1 Converter] No SafeZone config found — skipping");
            return;
        }

        TPV1SafeZoneConfig v1Sz = new TPV1SafeZoneConfig();
        TraderXJsonLoader<TPV1SafeZoneConfig>.LoadFromFile(TPV1_SAFEZONE_CONFIG, v1Sz);

        if (!v1Sz)
        {
            GetTraderXLogger().LogError("[TPV1 Converter] Failed to load SafeZone config");
            return;
        }

        TXSafeZoneSettings txSz = new TXSafeZoneSettings();
        txSz.version = "1.0.0";
        txSz.enableAfkDisconnect = v1Sz.EnableAfkDisconnect;
        txSz.kickAfterDelay = v1Sz.KickAfterDelay;
        txSz.cleanupTimer = v1Sz.CleanUpTimer;
        txSz.msgOnEnteringZone = v1Sz.MsgEnterZone;
        txSz.msgOnLeavingZone = v1Sz.MsgExitZone;

        // Admin UIDs
        if (v1Sz.SZSteamUIDs)
        {
            foreach (string uid : v1Sz.SZSteamUIDs)
            {
                txSz.adminSteamUIDs.Insert(uid);
            }
        }

        // Objects to delete
        if (v1Sz.ObjectsToDelete)
        {
            foreach (string obj : v1Sz.ObjectsToDelete)
            {
                txSz.objectsToDelete.Insert(obj);
            }
        }

        // Convert zone locations: V1 uses 2D (X,Y) + Radius; TraderX uses 3D vector + radius
        int zoneId = 1;
        if (v1Sz.SafeAreaLocation)
        {
            foreach (TPV1SafeZoneLocation v1Zone : v1Sz.SafeAreaLocation)
            {
                // V1 stores X and Y as ground-plane coordinates; height is 0
                vector position = Vector(v1Zone.X, 0, v1Zone.Y);
                int radius = v1Zone.Radius;

                // Use the countdown from V1 as the leaving zone countdown
                if (v1Zone.Countdown > 0)
                    txSz.leaveZoneCountdown = v1Zone.Countdown;

                string zoneName = v1Zone.SafeZoneStatut;
                if (zoneName == string.Empty)
                    zoneName = "Zone " + zoneId;

                txSz.locations.Insert(new TXSafeZoneLocation(true, zoneId, zoneName, position, radius, true, new array<string>()));
                zoneId++;
            }
        }

        TXSafeZoneSettings.CheckDirectories();
        JsonFileLoader<TXSafeZoneSettings>.JsonSaveFile(Safe_Zone_Config_Filename, txSz);

        GetTraderXLogger().LogInfo(string.Format("[TPV1 Converter] SafeZone converted: %1 zones, %2 admins", txSz.locations.Count(), txSz.adminSteamUIDs.Count()));
    }

    // ========================================================================
    // Phase 6: Migrate player bank accounts
    // ========================================================================
    private void MigrateBankAccounts()
    {
        if (!FileExist(TPV1_BANK_DB_DIR))
        {
            GetTraderXLogger().LogInfo("[TPV1 Converter] No bank database found — skipping");
            return;
        }

        // Find all Account_*.json files
        string fileName;
        FileAttr fileAttr;
        FindFileHandle handle = FindFile(TPV1_BANK_DB_DIR + "Account_*.json", fileName, fileAttr, FindFileFlags.ALL);

        if (!handle)
        {
            GetTraderXLogger().LogInfo("[TPV1 Converter] No bank accounts found");
            return;
        }

        int accountCount = 0;

        // Process each account
        if (fileName != string.Empty)
        {
            MigrateSingleBankAccount(TPV1_BANK_DB_DIR + fileName);
            accountCount++;
        }

        while (FindNextFile(handle, fileName, fileAttr))
        {
            if (fileName != string.Empty)
            {
                MigrateSingleBankAccount(TPV1_BANK_DB_DIR + fileName);
                accountCount++;
            }
        }

        CloseFindFile(handle);
        GetTraderXLogger().LogInfo(string.Format("[TPV1 Converter] Migrated %1 bank accounts", accountCount));
    }

    private void MigrateSingleBankAccount(string filePath)
    {
        TPV1BankAccount v1Account = new TPV1BankAccount();
        TraderXJsonLoader<TPV1BankAccount>.LoadFromFile(filePath, v1Account, false);

        if (!v1Account || v1Account.SteamID64 == string.Empty)
            return;

        // Migrate player licenses to TraderX license system
        if (v1Account.Licences && v1Account.Licences.Count() > 0)
        {
            TraderXPlayerLicenses playerLicenses = new TraderXPlayerLicenses(v1Account.SteamID64, v1Account.Name);

            foreach (string licName : v1Account.Licences)
            {
                playerLicenses.AddLicense(new TraderXLicense(licName, string.Empty));
            }

            // Save to TraderX license database
            TraderXLicenseRepository.MakeDirectoryIFNotExist();
            string licensePath = string.Format(TRADERX_PLAYER_LICENSES_FILE, v1Account.SteamID64);
            JsonFileLoader<TraderXPlayerLicenses>.JsonSaveFile(licensePath, playerLicenses);
        }

        int licCount = 0;
        if (v1Account.Licences)
            licCount = v1Account.Licences.Count();

        GetTraderXLogger().LogDebug(string.Format("[TPV1 Converter] Migrated bank account: %1 (balance: %2, licenses: %3)",
            v1Account.Name, v1Account.MoneyAmount, licCount));
    }

    // ========================================================================
    // Phase 7: Backup original V1 files
    // ========================================================================
    private void BackupV1Files()
    {
        if (!FileExist(TPV1_BACKUP_DIR))
            MakeDirectory(TPV1_BACKUP_DIR);

        BackupFile(TPV1_GENERAL_CONFIG, TPV1_BACKUP_DIR + "TraderPlusGeneralConfig.json");
        BackupFile(TPV1_PRICE_CONFIG, TPV1_BACKUP_DIR + "TraderPlusPriceConfig.json");
        BackupFile(TPV1_IDS_CONFIG, TPV1_BACKUP_DIR + "TraderPlusIDsConfig.json");
        BackupFile(TPV1_BANKING_CONFIG, TPV1_BACKUP_DIR + "TraderPlusBankingConfig.json");
        BackupFile(TPV1_SAFEZONE_CONFIG, TPV1_BACKUP_DIR + "TraderPlusSafeZoneConfig.json");
        BackupFile(TPV1_GARAGE_CONFIG, TPV1_BACKUP_DIR + "TraderPlusGarageConfig.json");

        GetTraderXLogger().LogInfo("[TPV1 Converter] Original V1 configs backed up to: " + TPV1_BACKUP_DIR);
    }

    private void BackupFile(string source, string destination)
    {
        if (FileExist(source))
        {
            CopyFile(source, destination);
            DeleteFile(source);
        }
    }

    // ========================================================================
    // Ensure all TraderX output directories exist
    // ========================================================================
    private void EnsureDirectories()
    {
        if (!FileExist(TRADERX_CONFIG_ROOT_SERVER))
            MakeDirectory(TRADERX_CONFIG_ROOT_SERVER);
        if (!FileExist(TRADERX_CONFIG_DIR_SERVER))
            MakeDirectory(TRADERX_CONFIG_DIR_SERVER);
        if (!FileExist(TRADERX_CATEGORIES_DIR))
            MakeDirectory(TRADERX_CATEGORIES_DIR);
        if (!FileExist(TRADERX_PRODUCTS_DIR))
            MakeDirectory(TRADERX_PRODUCTS_DIR);
        if (!FileExist(TRADERX_DB_DIR_SERVER))
            MakeDirectory(TRADERX_DB_DIR_SERVER);
        if (!FileExist(TRADERX_STOCK_DIR))
            MakeDirectory(TRADERX_STOCK_DIR);
        if (!FileExist(TRADERX_PLAYER_LICENSES_DIR))
            MakeDirectory(TRADERX_PLAYER_LICENSES_DIR);
    }
}
