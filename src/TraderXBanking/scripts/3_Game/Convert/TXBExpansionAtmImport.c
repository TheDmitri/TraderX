// Expansion ATM balances, staged by the TraderX Expansion importer into
// $profile:TraderX/TraderXBanking/ExpansionImport/<id>.json. Expansion names them after
// identity.GetId() (a one-way hash of the steam id), so they can only be matched when the player
// logs in. Each file is credited once, then renamed to .imported.
class TXBExpansionAtm
{
	string PlayerID;
	int MoneyDeposited;
}

class TXBExpansionAtmImport
{
	static const string DIR = "$profile:TraderX/TraderXBanking/ExpansionImport/";

	static void TryImport(PlayerIdentity identity, TXBSettings settings)
	{
		if (!identity || !settings || !FileExist(DIR))
			return;

		string id = identity.GetId();
		string urlSafeId = id;
		urlSafeId.Replace("/", "_");
		urlSafeId.Replace("+", "-");
		array<string> candidates = { id, urlSafeId };

		foreach (string candidate : candidates)
		{
			string path = DIR + candidate + ".json";
			if (!FileExist(path))
				continue;

			TXBExpansionAtm atm = new TXBExpansionAtm();
			string error;
			if (!JsonFileLoader<TXBExpansionAtm>.LoadFile(path, atm, error))
			{
				GetTXBLog().LogError("[TXB Migration] Could not read Expansion ATM file " + path + " " + error);
				return;
			}

			TXBAccountData account = TXBAccountData.Load(identity.GetPlainId(), identity.GetName(), settings.defaultMaxBalance, settings.defaultStartBalance);
			account.balance = account.balance + atm.MoneyDeposited;
			account.Save();

			CopyFile(path, path + ".imported");
			if (FileExist(path + ".imported"))
				DeleteFile(path);

			GetTXBLog().LogInfo("[TXB Migration] Credited " + atm.MoneyDeposited + " from Expansion ATM to " + identity.GetPlainId());
			return;
		}
	}
}
