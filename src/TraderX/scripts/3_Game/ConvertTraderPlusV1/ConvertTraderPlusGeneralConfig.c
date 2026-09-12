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

	static void GetCategoriesFromId(int id, out array<string> categories)	{
		oldTraderXIDsSettings settings = new oldTraderXIDsSettings();
		string idsPath = TraderXLegacyConfigPath.Resolve("TraderXIDsConfig.json", "TraderPlusIDsConfig.json");
		if (idsPath != string.Empty) {
			TraderXJsonLoader<oldTraderXIDsSettings>.LoadFromFile(idsPath, settings);
		}

        foreach(oldTraderXIDs tpId: settings.IDs)
        {
            if(tpId.Id == id)
            {
                categories = tpId.Categories;
            }
        }
	}

	static void GetCurrenciesFromId(int id, out array<string> currencies)	{
		oldTraderXIDsSettings settings = new oldTraderXIDsSettings();
		string idsPath = TraderXLegacyConfigPath.Resolve("TraderXIDsConfig.json", "TraderPlusIDsConfig.json");
		if (idsPath != string.Empty) {
			TraderXJsonLoader<oldTraderXIDsSettings>.LoadFromFile(idsPath, settings);
		}

        foreach(oldTraderXIDs tpId: settings.IDs)
        {
            if(tpId.Id == id)
            {
                currencies = tpId.CurrenciesAccepted;
            }
        }
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
	ref TraderXStates				       AcceptedStates;
	bool                                   StoreOnlyToPristineState;
	ref array<ref oldTraderXCurrency>      Currencies;
	ref array<ref oldTraderXData>	       Traders;
	ref array<ref oldTraderXObject>		   TraderObjects;

	// Paths the configs were actually loaded from — archived after a successful convert.
	string generalSourcePath = "";

	void TraderXGeneralSettings()
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

		newSettings.acceptedStates = AcceptedStates;
		
		// Convert old currencies to new currency type collection
		ConvertCurrenciesToNewFormat(newSettings);

        foreach(oldTraderXObject traderObject: TraderObjects)
        {
            newSettings.traderObjects.Insert(new TraderXObject(traderObject.ObjectName, traderObject.Position, traderObject.Orientation));
        }

        foreach(oldTraderXData oldTrader: Traders)
        {
            array<ref TraderXLoadoutItem> loadouts = new array<ref TraderXLoadoutItem>();
            foreach(string cloth: oldTrader.Clothes)
            {
                loadouts.Insert(new TraderXLoadoutItem(cloth));
            }

            array<string> categoryNames = new array<string>();
            oldTraderXIDsSettings.GetCategoriesFromId(oldTrader.Id, categoryNames);
            newSettings.traders.Insert(new TraderXNpc(oldTrader.Id, oldTrader.Name, oldTrader.GivenName, oldTrader.Role, oldTrader.Position, oldTrader.Orientation, loadouts, GetCategoriesIdByName(categoryNames), new array<string>()));
        }

		TraderXSettingsRepository.Save(newSettings);

        // Archive the consumed sources so the migration won't run again.
        TraderXLegacyConfigPath.Archive(generalSourcePath, "TraderPlusGeneralConfig.json");
        string idsPath = TraderXLegacyConfigPath.Resolve("TraderXIDsConfig.json", "TraderPlusIDsConfig.json");
        TraderXLegacyConfigPath.Archive(idsPath, "TraderPlusIDsConfig.json");
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
                className.Trim();
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
