// TraderXBanking - Ledger Entry Model
// One settled transaction recorded on a player's account. JSON-serialisable.
// Stored server-side only, streamed to client on request via TXBRPCService.
//
// Timestamp is an int in YYYYMMDDhhmmss form - monotonic, sortable, human-readable
// (e.g. 20260417142238). Avoids EnforceScript-unfriendly epoch-second math.

class TXBLedgerEntry
{
	string id;
	int timestampUtc;
	int type;
	string counterpartyName;
	string counterpartyUid;
	int amount;
	int fee;
	int balanceAfter;
	string location;
	string note;

	void TXBLedgerEntry()
	{
		id = "";
		timestampUtc = 0;
		type = 0;
		counterpartyName = "";
		counterpartyUid = "";
		amount = 0;
		fee = 0;
		balanceAfter = 0;
		location = "";
		note = "";
	}

	static ref TXBLedgerEntry Create(int entryType, int amt, int fees, int balAfter, string cpartyName, string cpartyUid, string loc, string memo)
	{
		ref TXBLedgerEntry entry = new TXBLedgerEntry();
		entry.id = GenerateId();
		entry.timestampUtc = GetUtcStamp();
		entry.type = entryType;
		entry.amount = amt;
		entry.fee = fees;
		entry.balanceAfter = balAfter;
		entry.counterpartyName = cpartyName;
		entry.counterpartyUid = cpartyUid;
		entry.location = loc;
		entry.note = memo;
		return entry;
	}

	static string GenerateId()
	{
		int a = Math.RandomInt(0, 65535);
		int b = Math.RandomInt(0, 65535);
		int c = Math.RandomInt(0, 65535);
		return "TX-" + a.ToString() + "-" + b.ToString() + "-" + c.ToString();
	}

	// Current UTC as Unix epoch seconds (sortable, fits int32 until 2038).
	static int GetUtcStamp()
	{
		return TXBTimeUtil.Now();
	}
}

class TXBLedgerFile
{
	string version;
	string playerUID;
	ref array<ref TXBLedgerEntry> entries;

	void TXBLedgerFile()
	{
		version = TXBConstants.LEDGER_VERSION;
		playerUID = "";
		entries = new array<ref TXBLedgerEntry>();
	}
}
