class oldTraderXObject
{
	string ObjectName;
	vector Position, Orientation;
}

class oldTraderXIDs
{
	int               Id=-1;
	ref array<string> Categories;
	ref TStringArray  LicencesRequired;
	ref TStringArray  CurrenciesAccepted;

  void oldTraderXIDs()
  {
    Categories = new array<string>();
	LicencesRequired = new TStringArray();
	CurrenciesAccepted = new TStringArray();
  }
};


class oldTraderXIDsSettings
{
	string Version = "";
	ref array<ref oldTraderXIDs>IDs;

	void oldTraderXIDsSettings()
	{
		IDs   = new array<ref oldTraderXIDs>;
	}

	static oldTraderXIDsSettings Load()
	{
		oldTraderXIDsSettings settings = new oldTraderXIDsSettings();
		string idsPath = TraderXLegacyConfigPath.Resolve("TraderXIDsConfig.json", "TraderPlusIDsConfig.json");
		if (idsPath != string.Empty)
			TraderXJsonLoader<oldTraderXIDsSettings>.LoadFromFile(idsPath, settings);
		return settings;
	}

	oldTraderXIDs Find(int id)
	{
		foreach (oldTraderXIDs tpId : IDs)
		{
			if (tpId && tpId.Id == id)
				return tpId;
		}
		return null;
	}
}

class oldTraderXData
{
	int 			Id;
    string  		Name;
	string          GivenName;
	string          Role;
	vector			Position;
	vector			Orientation;
	ref array<string> Clothes;
};

// TraderPlus key names (TraderXStates uses camelCase keys, so it cannot read them directly).
class oldTraderPlusStates
{
	bool  AcceptWorn;
	bool  AcceptDamaged;
	bool  AcceptBadlyDamaged;
	float CoefficientWorn = 0.75;
	float CoefficientDamaged = 0.5;
	float CoefficientBadlyDamaged = 0.25;
};

class oldTraderXCurrency
{
	string			ClassName;
	int				Value;
};


class oldTraderXGeneralSettings
{
	string 								   Version = "";
	bool                                   ConvertTraderConfigToTraderX;
	bool                                   ConvertTraderConfigToTraderXWithStockBasedOnCE;
	bool                                   UseGarageToTradeCar;
	bool                                   DisableHeightFailSafeForReceiptDeployment;
	bool                                   HideInsuranceBtn;
	bool                                   HideGarageBtn;
	bool                                   HideLicenceBtn;
	bool                                   EnableShowAllPrices;
	bool                                   EnableShowAllCheckBox;
	bool                                   IsReceiptTraderOnly;
	bool                                   IsReceiptSaveLock;
	bool                                   IsReceiptSaveAttachment;
	bool                                   IsReceiptSaveCargo;
	float                                  LockPickChance;
	string                                 LicenceKeyWord;
	ref TStringArray                       Licences;
	ref oldTraderPlusStates			       AcceptedStates;
	bool                                   StoreOnlyToPristineState;
	ref array<ref oldTraderXCurrency>      Currencies;
	ref array<ref oldTraderXData>	       Traders;
	ref array<ref oldTraderXObject>		   TraderObjects;

	// Paths the configs were actually loaded from — archived after a successful convert.
	string generalSourcePath = "";

	void oldTraderXGeneralSettings()
	{
		Licences = new TStringArray();
		Traders = new array<ref oldTraderXData>();
		TraderObjects = new array<ref oldTraderXObject>();
		Currencies = new array<ref oldTraderXCurrency>();
	}

    array<string> GetCategoriesIdByName(array<string> categoryNames)
    {
        array<string> categoriesId = new array<string>();

        if(!TraderXCategoryRepository.GetCategories()){
            GetTraderXLogger().LogError("GetCategoriesIdByName GetCategories() is null...");
            return categoriesId;
        }

        foreach(string categoryName: categoryNames)
        {
			string categoryId = TraderXCategoryRepository.GetCategoryIdByName(categoryName);
            if(categoryId != string.Empty)
			{
				categoriesId.Insert(categoryId);
			}
			else
			{
				TraderXMigrationReport.Skipped("TraderPlus", "category '" + categoryName + "' is listed for a trader but has no products in TraderPlusPriceConfig.json");
			}
        }

		GetTraderXLogger().LogDebug("categoriesId count: " + categoriesId.Count());

        return categoriesId;
    }

    void ConvertToNewConfig()
    {
        TraderXGeneralSettings newSettings = new TraderXGeneralSettings();

        newSettings.version = TRADERX_CURRENT_VERSION;
		newSettings.serverID = TraderXUuid.GenerateUUID();
		foreach(string license: Licences)
		{
			newSettings.licenses.Insert(new TraderXLicense(license, string.Empty));
		}

		if (AcceptedStates)
		{
			newSettings.acceptedStates = new TraderXStates(AcceptedStates.AcceptWorn, AcceptedStates.AcceptDamaged, AcceptedStates.AcceptBadlyDamaged);
			newSettings.acceptedStates.coefficientWorn = AcceptedStates.CoefficientWorn;
			newSettings.acceptedStates.coefficientDamaged = AcceptedStates.CoefficientDamaged;
			newSettings.acceptedStates.coefficientBadlyDamaged = AcceptedStates.CoefficientBadlyDamaged;
		}

		// Convert old currencies to new currency type collection
		ConvertCurrenciesToNewFormat(newSettings);

        foreach(oldTraderXObject traderObject: TraderObjects)
        {
            newSettings.traderObjects.Insert(new TraderXObject(traderObject.ObjectName, traderObject.Position, traderObject.Orientation));
        }

        oldTraderXIDsSettings ids = oldTraderXIDsSettings.Load();
        foreach(oldTraderXData oldTrader: Traders)
        {
            ConvertTrader(oldTrader, ids, newSettings);
        }

		ReportSettingsWithoutEquivalent();

		TraderXSettingsRepository.Save(newSettings);
		if (ConvertTraderPlusPlayerData.Convert(newSettings))
			TraderXSettingsRepository.Save(newSettings);

        // Archive the consumed sources so the migration won't run again.
        TraderXLegacyConfigPath.Archive(generalSourcePath, "TraderPlusGeneralConfig.json");
        string idsPath = TraderXLegacyConfigPath.Resolve("TraderXIDsConfig.json", "TraderPlusIDsConfig.json");
        TraderXLegacyConfigPath.Archive(idsPath, "TraderPlusIDsConfig.json");
    }

    // TraderPlus trader ids: >= 0 trade (categories from TraderPlusIDsConfig), -2 bank ATM,
    // -3 garage, -4 stash. TraderX uses fixed npc ids 126 (bank) and 125 (garage).
    private void ConvertTrader(oldTraderXData oldTrader, oldTraderXIDsSettings ids, TraderXGeneralSettings newSettings)
    {
        if (oldTrader.Id == -4)
        {
            TraderXMigrationReport.Skipped("TraderPlus", "stash trader '" + oldTrader.GivenName + "' (TraderX has no safezone stash)");
            return;
        }

        array<ref TraderXLoadoutItem> loadouts = new array<ref TraderXLoadoutItem>();
        foreach(string cloth: oldTrader.Clothes)
        {
            loadouts.Insert(new TraderXLoadoutItem(cloth));
        }

        oldTraderXIDs idEntry = ids.Find(oldTrader.Id);
        array<string> categoriesId = new array<string>();
        array<string> currencyTypes = new array<string>();
        if (idEntry && oldTrader.Id >= 0)
        {
            categoriesId = GetCategoriesIdByName(idEntry.Categories);
            currencyTypes = GetCurrencyTypesFromClassNames(idEntry.CurrenciesAccepted);
        }

        TraderXNpc npc = new TraderXNpc(oldTrader.Id, oldTrader.Name, oldTrader.GivenName, oldTrader.Role, oldTrader.Position, oldTrader.Orientation, loadouts, categoriesId, currencyTypes);
        if (oldTrader.Id == -2)
            npc.npcId = 126;
        else if (oldTrader.Id == -3)
            npc.npcId = 125;

        if (idEntry && oldTrader.Id >= 0 && idEntry.LicencesRequired)
        {
            array<string> licenses = new array<string>();
            licenses.Copy(idEntry.LicencesRequired);
            npc.SetLicensesRequired(licenses);
        }

        if (oldTrader.Id >= 0 && !idEntry)
            TraderXMigrationReport.Skipped("TraderPlus", "trader '" + oldTrader.GivenName + "' has id " + oldTrader.Id + " which is not defined in TraderPlusIDsConfig.json (no categories)");

        newSettings.traders.Insert(npc);
        TraderXMigrationReport.Info("TraderPlus", string.Format("trader '%1' (TraderPlus id %2) -> TraderX npcId %3, %4 categories", oldTrader.GivenName, oldTrader.Id, npc.npcId, categoriesId.Count()));
    }

    // TraderPlus accepts currency class names per trader; TraderX accepts currency types.
    private array<string> GetCurrencyTypesFromClassNames(array<string> classNames)
    {
        array<string> types = new array<string>();
        if (!classNames)
            return types;

        foreach (string className : classNames)
        {
            string currencyType = GetCurrencyTypeFromClassName(className);
            if (types.Find(currencyType) == -1)
                types.Insert(currencyType);
        }
        return types;
    }

    private void ReportSettingsWithoutEquivalent()
    {
        TraderXMigrationReport.Skipped("TraderPlus", "receipts and insurance (UseGarageToTradeCar, IsReceipt*, DisableHeightFailSafeForReceiptDeployment, TraderPlusInsuranceConfig.json): TraderX sells vehicles through parking spots");
        TraderXMigrationReport.Skipped("TraderPlus", "menu button toggles (HideInsuranceBtn, HideGarageBtn, HideLicenceBtn, EnableShowAllPrices, EnableShowAllCheckBox)");
        if (StoreOnlyToPristineState)
            TraderXMigrationReport.Skipped("TraderPlus", "StoreOnlyToPristineState: TraderX stock has no item condition");
        if (LicenceKeyWord != string.Empty)
            TraderXMigrationReport.Approx("TraderPlus", "LicenceKeyWord '" + LicenceKeyWord + "': in TraderX a licence is bought through a product whose class name equals the licence name");
        TraderXMigrationReport.Approx("TraderPlus", string.Format("LockPickChance %1: set lockpickChance in TraderX/TraderXCarLock/TXCLSettings.json", LockPickChance));
    }

    private void ConvertCurrenciesToNewFormat(TraderXGeneralSettings newSettings)
    {
        if (!Currencies || Currencies.Count() == 0) {
            GetTraderXLogger().LogInfo("No old currencies to convert, using default currency setup");
            return;
        }
        
        TraderXCurrencyTypeCollection currencyCollection = new TraderXCurrencyTypeCollection();
        
        // Group currencies by value to determine currency types
        map<int, ref array<string>> currenciesByValue = new map<int, ref array<string>>();
        
        foreach(oldTraderXCurrency oldCurrency: Currencies)
        {
            // Split comma-separated class names
            TStringArray classNames = new TStringArray();
            oldCurrency.ClassName.Split(",", classNames);
            
            if (!currenciesByValue.Contains(oldCurrency.Value)) {
                currenciesByValue.Set(oldCurrency.Value, new array<string>());
            }
            
            foreach(string className: classNames)
            {
                className.TrimInPlace();
                if (className != string.Empty) {
                    currenciesByValue.Get(oldCurrency.Value).Insert(className);
                }
            }
        }
        
        // Create currency types based on class name patterns
        map<string, ref TraderXCurrencyType> currencyTypes = new map<string, ref TraderXCurrencyType>();
        
        foreach(int value, array<string> clasNames: currenciesByValue)
        {
            foreach(string clasName: clasNames)
            {
                string currencyTypeName = GetCurrencyTypeFromClassName(clasName);
                
                if (!currencyTypes.Contains(currencyTypeName)) {
                    currencyTypes.Set(currencyTypeName, new TraderXCurrencyType(currencyTypeName));
                }
                
                currencyTypes.Get(currencyTypeName).AddCurrency(clasName, value);
            }
        }
        
        // Add all currency types to collection
        foreach(string typeName, TraderXCurrencyType currencyType: currencyTypes)
        {
            currencyCollection.currencyTypes.Insert(currencyType);
        }
        
        // Save the currency collection
        TraderXCurrencyRepository.Save(currencyCollection);
        GetTraderXLogger().LogInfo("Converted " + Currencies.Count() + " old currencies to new format");
    }
    
    private string GetCurrencyTypeFromClassName(string className)
    {
        if (className.Contains("Dollar") || className.Contains("USD")) {
            return "USD";
        } else if (className.Contains("Euro") || className.Contains("EUR")) {
            return "EUR";
        } else if (className.Contains("Ruble") || className.Contains("RUB")) {
            return "RUB";
        } else if (className.Contains("DIM")) {
            return "DIM";
        }
        
        // Default fallback
        return "UNKNOWN";
    }
    
	static void ConvertOldConfigToNew()	{
		string path = TraderXLegacyConfigPath.Resolve("TraderXGeneralConfig.json", "TraderPlusGeneralConfig.json");
		if (path == string.Empty)
			return;

		oldTraderXGeneralSettings settings = new oldTraderXGeneralSettings();
		GetTraderXLogger().LogInfo("[TraderX Migration] Loading legacy general config: " + path);
		TraderXJsonLoader<oldTraderXGeneralSettings>.LoadFromFile(path, settings);
		settings.generalSourcePath = path;
		settings.ConvertToNewConfig();
	}
}
