// TraderXBanking - Player Bank Account Data
// JSON-serializable per-player account stored on disk

class TXBAccountData
{
	string version = TXB_ACCOUNT_VERSION;
	string playerUID;
	string playerName;
	int balance;
	int maxBalance;

	void TXBAccountData()
	{
		playerUID = "";
		playerName = "";
		balance = 0;
		maxBalance = 1000000;
	}

	static string GetAccountFilePath(string uid)
	{
		return TXB_DB_DIR + "Account_" + uid + ".json";
	}

	void Save()
	{
		if (!FileExist(TXB_DB_DIR))
			MakeDirectory(TXB_DB_DIR);

		string filePath = GetAccountFilePath(playerUID);
		JsonFileLoader<TXBAccountData>.JsonSaveFile(filePath, this);
	}

	static ref TXBAccountData Load(string uid, string name, int defaultMax, int defaultStart)
	{
		if (!FileExist(TXB_DB_DIR))
			MakeDirectory(TXB_DB_DIR);

		string filePath = GetAccountFilePath(uid);
		ref TXBAccountData account;

		if (FileExist(filePath))
		{
			account = new TXBAccountData();
			JsonFileLoader<TXBAccountData>.JsonLoadFile(filePath, account);

			if (account.version != TXB_ACCOUNT_VERSION)
			{
				account.version = TXB_ACCOUNT_VERSION;
				account.Save();
			}
		}
		else
		{
			account = new TXBAccountData();
			account.playerUID = uid;
			account.playerName = name;
			account.balance = defaultStart;
			account.maxBalance = defaultMax;
			account.Save();
			DZLog().LogInfo("[TXB] New account created for " + name + " (" + uid + ")");
		}

		return account;
	}

	static bool AccountExists(string uid)
	{
		return FileExist(GetAccountFilePath(uid));
	}
}
