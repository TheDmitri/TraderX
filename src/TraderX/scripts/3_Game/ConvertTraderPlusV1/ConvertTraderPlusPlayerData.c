// TraderPlus V1 player data -> TraderX: owned licences, bank balances and trader stock.
// Runs right after the config import (same one-time TraderPlus import), reading the TraderPlus
// databases in place and never modifying them.
// If a TraderPlus database folder exists but FindFile cannot list it (DayZ 1.30 with an absolute
// -profiles path), that part is recorded in PENDING_FILE and retried on the next boots by
// RetryPending(); the config import itself is not re-run (TraderXGeneralSettings.json now exists).

// $profile:TraderPlus/TraderPlusBankDatabase/Account_<steam64>.json
class oldTraderPlusAccount
{
	string Version;
	string SteamID64;
	string Name;
	int MoneyAmount;
	int MaxAmount;
	ref array<string> Licences;
	ref map<string, string> Insurances;
}

// Same JSON layout as TraderXBanking's TXBAccountData (core does not depend on the banking PBO).
class TraderXImportedBankAccount
{
	string version = "1.0";
	string playerUID;
	string playerName;
	int balance;
	int maxBalance;
}

// $profile:TraderPlus/TraderPlusDatabase/ID_<n>/<Category>.json, lines "ClassName qty healthState".
class oldTraderPlusStockCategory
{
	string CategoryName;
	ref array<string> Products;
}

// Player-data parts still to import, with the product mapping the stock import needs.
class TraderXTraderPlusPendingPlayerData
{
	bool accounts;
	bool stock;
	ref map<string, string> productIdByKey;
	ref map<string, int> maxStockByProductId;
}

class ConvertTraderPlusPlayerData
{
	static const string TRADERPLUS_BANK_DIR = "$profile:TraderPlus/TraderPlusBankDatabase/";
	static const string TRADERPLUS_STOCK_DIR = "$profile:TraderPlus/TraderPlusDatabase/";
	static const string TRADERX_BANK_ACCOUNTS_DIR = "$profile:TraderX/TraderXBanking/Accounts/";
	static const string PENDING_FILE = "$profile:TraderX/TraderXConfig/.pending_traderplus_players.json";

	// Filled by the price import: "<category>|<classname>" (lowercase) -> productId / maxStock.
	static ref map<string, string> s_ProductIdByKey = new map<string, string>();
	static ref map<string, int> s_MaxStockByProductId = new map<string, int>();

	static string ProductKey(string categoryName, string className)
	{
		string key = categoryName + "|" + className;
		key.ToLower();
		return key;
	}

	static void RegisterProduct(string categoryName, TraderXProduct product)
	{
		s_ProductIdByKey.Set(ProductKey(categoryName, product.className), product.productId);
		s_MaxStockByProductId.Set(product.productId, product.maxStock);
	}

	protected static bool s_SettingsChanged;

	// Returns true when the trader settings were changed (licences added) and must be saved again.
	static bool Convert(TraderXGeneralSettings settings)
	{
		s_SettingsChanged = false;
		bool accountsDone = ConvertAccounts(settings);
		bool stockDone = ConvertStock();
		SavePending(!accountsDone, !stockDone);
		return s_SettingsChanged;
	}

	// Every boot: re-runs the player-data parts a previous import could not list.
	static void RetryPending()
	{
		if (!FileExist(PENDING_FILE))
			return;

		TraderXTraderPlusPendingPlayerData pending = new TraderXTraderPlusPendingPlayerData();
		string error;
		if (!JsonFileLoader<TraderXTraderPlusPendingPlayerData>.LoadFile(PENDING_FILE, pending, error))
		{
			TraderXMigrationReport.Error("TraderPlus", "could not read " + PENDING_FILE + " (" + error + ") - TraderPlus player data retry skipped");
			TraderXMigrationReport.Write("TraderPlus", true);
			return;
		}

		GetTraderXLogger().LogInfo("[TraderX Migration] Retrying the TraderPlus player data import (" + PENDING_FILE + ")");
		if (pending.productIdByKey)
			s_ProductIdByKey = pending.productIdByKey;
		if (pending.maxStockByProductId)
			s_MaxStockByProductId = pending.maxStockByProductId;

		s_SettingsChanged = false;
		bool accountsDone = true;
		if (pending.accounts)
		{
			TraderXGeneralSettings settings = new TraderXGeneralSettings();
			string settingsError;
			if (!JsonFileLoader<TraderXGeneralSettings>.LoadFile(TRADERX_CONFIG_FILE, settings, settingsError))
			{
				TraderXMigrationReport.Error("TraderPlus", "could not read " + TRADERX_CONFIG_FILE + " (" + settingsError + ") - TraderPlus accounts not imported, retried next boot");
				accountsDone = false;
			}
			else
			{
				accountsDone = ConvertAccounts(settings);
				if (s_SettingsChanged)
					TraderXSettingsRepository.Save(settings);
			}
		}

		bool stockDone = true;
		if (pending.stock)
			stockDone = ConvertStock();

		SavePending(!accountsDone, !stockDone);
		TraderXMigrationReport.Write("TraderPlus", true);
	}

	protected static void SavePending(bool accounts, bool stock)
	{
		if (!accounts && !stock)
		{
			if (FileExist(PENDING_FILE))
				DeleteFile(PENDING_FILE);
			return;
		}

		TraderXTraderPlusPendingPlayerData pending = new TraderXTraderPlusPendingPlayerData();
		pending.accounts = accounts;
		pending.stock = stock;
		pending.productIdByKey = s_ProductIdByKey;
		pending.maxStockByProductId = s_MaxStockByProductId;
		if (!FileExist(TRADERX_CONFIG_ROOT_SERVER))
			MakeDirectory(TRADERX_CONFIG_ROOT_SERVER);
		if (!FileExist(TRADERX_CONFIG_DIR_SERVER))
			MakeDirectory(TRADERX_CONFIG_DIR_SERVER);
		string error;
		JsonFileLoader<TraderXTraderPlusPendingPlayerData>.SaveFile(PENDING_FILE, pending, error);
	}

	// The folder exists but FindFile returned nothing: on DayZ 1.30 it cannot list $profile: when
	// -profiles is absolute / outside the server folder. Nothing was imported, so a retry is safe.
	protected static void ReportListingFailed(string dir, string what)
	{
		string message = what + " in " + dir + ": FindFile could not list the folder (DayZ 1.30 with an absolute -profiles path?). Use a -profiles path relative to the server folder and restart - retried while " + PENDING_FILE + " exists (delete it if the folder is really empty)";
		GetTraderXLogger().LogError("[TraderX Migration] TraderPlus: " + message);
		TraderXMigrationReport.Skipped("TraderPlus", message);
	}

	// Bank balance -> TraderXBanking account, owned licences -> TraderX player licences.
	// Returns false when the TraderPlus bank folder exists but could not be listed.
	static bool ConvertAccounts(TraderXGeneralSettings settings)
	{
		string fileName;
		FileAttr attr;
		FindFileHandle handle = TraderXFileSystem.Find(TRADERPLUS_BANK_DIR + "Account_*.json", fileName, attr, FindFileFlags.ALL);
		if (!handle)
		{
			if (FileExist(TRADERPLUS_BANK_DIR))
			{
				ReportListingFailed(TRADERPLUS_BANK_DIR, "bank balances and owned licences");
				return false;
			}
			return true;
		}

		int accounts = 0;
		bool hasNext = true;
		while (hasNext)
		{
			if (ConvertAccount(TRADERPLUS_BANK_DIR + fileName, settings))
				accounts++;
			hasNext = FindNextFile(handle, fileName, attr);
		}
		CloseFindFile(handle);

		TraderXMigrationReport.Info("TraderPlus", accounts.ToString() + " player account(s): bank balance and owned licences imported");
		return true;
	}

	protected static bool ConvertAccount(string path, TraderXGeneralSettings settings)
	{
		oldTraderPlusAccount account = new oldTraderPlusAccount();
		string error;
		if (!JsonFileLoader<oldTraderPlusAccount>.LoadFile(path, account, error) || account.SteamID64 == string.Empty)
		{
			TraderXMigrationReport.Skipped("TraderPlus", "unreadable bank account " + path + " " + error);
			return false;
		}

		WriteBankAccount(account);

		if (account.Licences && account.Licences.Count() > 0)
		{
			TraderXPlayerLicenses licenses = TraderXLicenseRepository.Load(account.SteamID64, account.Name);
			foreach (string licenceName : account.Licences)
			{
				TraderXLicense license = settings.GetLicenseByName(licenceName);
				if (!license)
				{
					// Owned by a player but missing from the TraderPlus licence list: define it so the
					// player keeps it (the trader settings are saved again after the accounts pass).
					license = new TraderXLicense(licenceName, string.Empty);
					settings.licenses.Insert(license);
					s_SettingsChanged = true;
					TraderXMigrationReport.Info("TraderPlus", "licence '" + licenceName + "' owned by players but missing from TraderPlusGeneralConfig.json Licences - added to TraderX licences");
				}
				if (!licenses.HasLicense(license.licenseId))
					licenses.AddLicense(license);
			}
			TraderXLicenseRepository.Save(licenses);
		}

		if (account.Insurances && account.Insurances.Count() > 0)
			TraderXMigrationReport.Skipped("TraderPlus", "vehicle insurances of player " + account.SteamID64 + " (no insurance in TraderX)");

		return true;
	}

	protected static void WriteBankAccount(oldTraderPlusAccount account)
	{
		string path = TRADERX_BANK_ACCOUNTS_DIR + "Account_" + account.SteamID64 + ".json";
		if (FileExist(path))
		{
			TraderXMigrationReport.Skipped("TraderPlus", "bank account of " + account.SteamID64 + " already exists in TraderX - kept");
			return;
		}

		array<string> dirs = { "$profile:TraderX/", "$profile:TraderX/TraderXBanking/", TRADERX_BANK_ACCOUNTS_DIR };
		foreach (string dir : dirs)
		{
			if (!FileExist(dir))
				MakeDirectory(dir);
		}

		TraderXImportedBankAccount imported = new TraderXImportedBankAccount();
		imported.playerUID = account.SteamID64;
		imported.playerName = account.Name;
		imported.balance = account.MoneyAmount;
		imported.maxBalance = account.MaxAmount;
		string error;
		JsonFileLoader<TraderXImportedBankAccount>.SaveFile(path, imported, error);
	}

	// TraderPlus keeps stock per trader id and item condition; TraderX keeps one stock per product.
	// Quantities are summed over ids and conditions, capped at the product's max stock.
	// Returns false when the TraderPlus stock folder exists but could not be listed.
	static bool ConvertStock()
	{
		map<string, int> stockByProductId = new map<string, int>();

		string dirName;
		FileAttr dirAttr;
		FindFileHandle dirs = TraderXFileSystem.Find(TRADERPLUS_STOCK_DIR + "ID_*", dirName, dirAttr, FindFileFlags.ALL);
		if (!dirs)
		{
			if (FileExist(TRADERPLUS_STOCK_DIR))
			{
				ReportListingFailed(TRADERPLUS_STOCK_DIR, "trader stock levels");
				return false;
			}
			return true;
		}

		bool hasDir = true;
		while (hasDir)
		{
			ReadStockDir(TRADERPLUS_STOCK_DIR + dirName + "/", stockByProductId);
			hasDir = FindNextFile(dirs, dirName, dirAttr);
		}
		CloseFindFile(dirs);

		if (!FileExist(TRADERX_STOCK_DIR))
			MakeDirectory(TRADERX_STOCK_DIR);

		foreach (string productId, int stock : stockByProductId)
		{
			int maxStock = s_MaxStockByProductId.Get(productId);
			if (maxStock >= 0 && stock > maxStock)
				stock = maxStock;
			TraderXProductStockRepository.Save(new TraderXProductStock(productId, stock));
		}

		TraderXMigrationReport.Info("TraderPlus", stockByProductId.Count().ToString() + " product stock level(s) imported");
		TraderXMigrationReport.Approx("TraderPlus", "stock is per trader id and item condition in TraderPlus, one shared stock per product in TraderX: quantities were summed");
		return true;
	}

	protected static void ReadStockDir(string dir, map<string, int> stockByProductId)
	{
		string fileName;
		FileAttr attr;
		FindFileHandle handle = TraderXFileSystem.Find(dir + "*.json", fileName, attr, FindFileFlags.ALL);
		if (!handle)
			return;

		bool hasNext = true;
		while (hasNext)
		{
			ReadStockFile(dir + fileName, stockByProductId);
			hasNext = FindNextFile(handle, fileName, attr);
		}
		CloseFindFile(handle);
	}

	protected static void ReadStockFile(string path, map<string, int> stockByProductId)
	{
		oldTraderPlusStockCategory category = new oldTraderPlusStockCategory();
		string error;
		if (!JsonFileLoader<oldTraderPlusStockCategory>.LoadFile(path, category, error) || !category.Products)
			return;

		foreach (string line : category.Products)
		{
			TStringArray parts = new TStringArray();
			line.Split(" ", parts);
			if (parts.Count() < 2)
				continue;

			string productId = s_ProductIdByKey.Get(ProductKey(category.CategoryName, parts[0]));
			if (productId == string.Empty)
			{
				TraderXMigrationReport.Skipped("TraderPlus", "stock of " + parts[0] + " in category '" + category.CategoryName + "' (no such product in the price config)");
				continue;
			}

			stockByProductId.Set(productId, stockByProductId.Get(productId) + parts[1].ToInt());
		}
	}
}
