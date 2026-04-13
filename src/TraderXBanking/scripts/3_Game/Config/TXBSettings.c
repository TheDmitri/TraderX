// TraderXBanking - Server Configuration
// JSON-serializable settings loaded from server profile

class TXBCurrencyEntry
{
	string className;
	int value;

	void TXBCurrencyEntry(string _className = "", int _value = 0)
	{
		className = _className;
		value = _value;
	}
}

class TXBSettings
{
	string version = TXB_CONFIG_VERSION;
	bool enableLogs = true;
	bool requireCreditCard = false;
	float transactionFeePercent = 0.05;
	int defaultStartBalance = 0;
	int defaultMaxBalance = 1000000;
	ref array<ref TXBCurrencyEntry> currencies = new array<ref TXBCurrencyEntry>();

	void TXBSettings()
	{
		if (currencies.Count() == 0)
		{
			// Default currency setup
			currencies.Insert(new TXBCurrencyEntry("TraderPlus_Money_Dollar1", 1));
			currencies.Insert(new TXBCurrencyEntry("TraderPlus_Money_Dollar5", 5));
			currencies.Insert(new TXBCurrencyEntry("TraderPlus_Money_Dollar10", 10));
			currencies.Insert(new TXBCurrencyEntry("TraderPlus_Money_Dollar25", 25));
			currencies.Insert(new TXBCurrencyEntry("TraderPlus_Money_Dollar100", 100));
			currencies.Insert(new TXBCurrencyEntry("TraderPlus_Money_Dollar1000", 1000));
		}
	}

	void Save()
	{
		if (!FileExist(TXB_CONFIG_DIR))
			MakeDirectory(TXB_CONFIG_DIR);

		JsonFileLoader<TXBSettings>.JsonSaveFile(TXB_CONFIG_FILE, this);
		DZLog().LogInfo("[TXB] Config saved to " + TXB_CONFIG_FILE);
	}

	static ref TXBSettings Load()
	{
		ref TXBSettings settings;

		if (!FileExist(TXB_CONFIG_DIR))
			MakeDirectory(TXB_CONFIG_DIR);

		if (FileExist(TXB_CONFIG_FILE))
		{
			settings = new TXBSettings();
			JsonFileLoader<TXBSettings>.JsonLoadFile(TXB_CONFIG_FILE, settings);
			DZLog().LogInfo("[TXB] Config loaded from " + TXB_CONFIG_FILE);

			if (settings.version != TXB_CONFIG_VERSION)
			{
				settings.version = TXB_CONFIG_VERSION;
				settings.Save();
			}
		}
		else
		{
			settings = new TXBSettings();
			settings.Save();
			DZLog().LogInfo("[TXB] Default config created at " + TXB_CONFIG_FILE);
		}

		return settings;
	}

	// Get currencies sorted by value descending (for withdrawal)
	array<ref TXBCurrencyEntry> GetCurrenciesSortedDesc()
	{
		array<ref TXBCurrencyEntry> sorted = new array<ref TXBCurrencyEntry>();
		foreach (TXBCurrencyEntry entry : currencies)
		{
			sorted.Insert(entry);
		}

		// Simple insertion sort descending by value
		for (int i = 1; i < sorted.Count(); i++)
		{
			ref TXBCurrencyEntry key = sorted[i];
			int j = i - 1;
			while (j >= 0 && sorted[j].value < key.value)
			{
				sorted[j + 1] = sorted[j];
				j = j - 1;
			}
			sorted[j + 1] = key;
		}

		return sorted;
	}

	// Get currencies sorted by value ascending (for removal)
	array<ref TXBCurrencyEntry> GetCurrenciesSortedAsc()
	{
		array<ref TXBCurrencyEntry> sorted = new array<ref TXBCurrencyEntry>();
		foreach (TXBCurrencyEntry entry : currencies)
		{
			sorted.Insert(entry);
		}

		for (int i = 1; i < sorted.Count(); i++)
		{
			ref TXBCurrencyEntry key = sorted[i];
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

	bool IsCurrency(string _className)
	{
		foreach (TXBCurrencyEntry entry : currencies)
		{
			if (entry.className == _className)
				return true;
		}
		return false;
	}

	int GetCurrencyValue(string _className)
	{
		foreach (TXBCurrencyEntry entry : currencies)
		{
			if (entry.className == _className)
				return entry.value;
		}
		return 0;
	}
}
