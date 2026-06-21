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

// Bank object configuration entry for standalone mode
// (when TraderX is not loaded, these static objects are spawned as bank terminals)
class TXBBankNpcEntry
{
	string className;        // Object class - must extend BuildingBase (e.g., "ATM", "Land_Terminal")
	vector position;         // World position
	vector orientation;      // Direction facing
	string name;             // Display name (optional)

	void TXBBankNpcEntry(string _className = "", vector _position = vector.Zero, vector _orientation = vector.Zero, string _name = "")
	{
		className = _className;
		position = _position;
		orientation = _orientation;
		name = _name;
	}
}

class TXBSettings
{
	string version = TXBConstants.CONFIG_VERSION;
	bool enableLogs = true;
	bool requireCreditCard = false;
	float transactionFeePercent = 0.05;
	int defaultStartBalance = 0;
	int defaultMaxBalance = 1000000;
	ref array<ref TXBCurrencyEntry> currencies = new array<ref TXBCurrencyEntry>();

	// Bank objects for standalone mode (spawned when TraderX is not present)
	// Must extend BuildingBase (static objects like ATMs, terminals, etc.)
	// If TraderX is loaded, these are ignored (use TraderX's NPC system for animated NPCs)
	ref array<ref TXBBankNpcEntry> bankNpcs = new array<ref TXBBankNpcEntry>();

	void TXBSettings()
	{
		if (currencies.Count() == 0)
		{
			// Default currency setup using TraderX money classes
			// Rubles
			currencies.Insert(new TXBCurrencyEntry("TraderX_Money_Ruble5000", 5000));
			currencies.Insert(new TXBCurrencyEntry("TraderX_Money_Ruble2000", 2000));
			currencies.Insert(new TXBCurrencyEntry("TraderX_Money_Ruble1000", 1000));
			currencies.Insert(new TXBCurrencyEntry("TraderX_Money_Ruble500", 500));
			currencies.Insert(new TXBCurrencyEntry("TraderX_Money_Ruble200", 200));
			currencies.Insert(new TXBCurrencyEntry("TraderX_Money_Ruble100", 100));
			currencies.Insert(new TXBCurrencyEntry("TraderX_Money_Ruble50", 50));
			currencies.Insert(new TXBCurrencyEntry("TraderX_Money_Ruble10", 10));
			currencies.Insert(new TXBCurrencyEntry("TraderX_Money_Ruble5", 5));
			currencies.Insert(new TXBCurrencyEntry("TraderX_Money_Ruble2_Coin", 2));
			currencies.Insert(new TXBCurrencyEntry("TraderX_Money_Ruble1_Coin", 1));
		}

		// Default ATM location at TraderPlus trader hub (Krasnostav airfield)
		if (bankNpcs.Count() == 0)
		{
			bankNpcs.Insert(new TXBBankNpcEntry("TraderPlus_BANK_ATM", Vector(11623.0, 57.91, 14676.0), Vector(-37.0, 0.0, 0.0), "ATM"));
		}
	}

	void Save()
	{
		if (!FileExist(TXBConstants.CONFIG_DIR))
			MakeDirectory(TXBConstants.CONFIG_DIR);

		JsonFileLoader<TXBSettings>.JsonSaveFile(TXBConstants.CONFIG_FILE, this);
		DZLog().LogInfo("[TXB] Config saved to " + TXBConstants.CONFIG_FILE);
	}

	static ref TXBSettings Load()
	{
		TXBSettings settings;

		if (!FileExist(TXBConstants.CONFIG_DIR))
			MakeDirectory(TXBConstants.CONFIG_DIR);

		if (FileExist(TXBConstants.CONFIG_FILE))
		{
			settings = new TXBSettings();
			JsonFileLoader<TXBSettings>.JsonLoadFile(TXBConstants.CONFIG_FILE, settings);
			DZLog().LogInfo("[TXB] Config loaded from " + TXBConstants.CONFIG_FILE);

			if (settings.version != TXBConstants.CONFIG_VERSION)
			{
				settings.version = TXBConstants.CONFIG_VERSION;
				settings.Save();
			}
		}
		else
		{
			settings = new TXBSettings();
			settings.Save();
			DZLog().LogInfo("[TXB] Default config created at " + TXBConstants.CONFIG_FILE);
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
