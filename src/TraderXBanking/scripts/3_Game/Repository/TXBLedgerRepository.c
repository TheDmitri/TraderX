// TraderXBanking - Ledger Repository
// Buffered append-only journal per player, trimmed to LEDGER_MAX_ENTRIES.
// Server-side only. Flush on threshold OR interval, whichever first.

class TXBLedgerRepository
{
	protected static ref map<string, ref TXBLedgerFile> m_Cache = new map<string, ref TXBLedgerFile>();
	protected static ref map<string, int> m_DirtyCount = new map<string, int>();
	protected static float m_FlushAccumulator = 0;

	static string GetLedgerFilePath(string uid)
	{
		return TXBConstants.LEDGER_DIR + "Ledger_" + uid + ".json";
	}

	static ref TXBLedgerFile Load(string uid)
	{
		if (m_Cache.Contains(uid))
			return m_Cache.Get(uid);

		if (!FileExist(TXBConstants.LEDGER_DIR))
			MakeDirectory(TXBConstants.LEDGER_DIR);

		string filePath = GetLedgerFilePath(uid);
		TXBLedgerFile file;

		if (FileExist(filePath))
		{
			file = new TXBLedgerFile();
			JsonFileLoader<TXBLedgerFile>.JsonLoadFile(filePath, file);

			if (file.version != TXBConstants.LEDGER_VERSION)
			{
				file.version = TXBConstants.LEDGER_VERSION;
			}
		}
		else
		{
			file = new TXBLedgerFile();
			file.playerUID = uid;
		}

		m_Cache.Set(uid, file);
		return file;
	}

	static void Append(string uid, ref TXBLedgerEntry entry)
	{
		if (!entry) return;

		TXBLedgerFile file = Load(uid);
		if (!file.entries)
			file.entries = new array<ref TXBLedgerEntry>();

		file.entries.Insert(entry);

		// Trim oldest if over cap
		while (file.entries.Count() > TXBConstants.LEDGER_MAX_ENTRIES)
		{
			file.entries.RemoveOrdered(0);
		}

		int dirty = 0;
		if (m_DirtyCount.Contains(uid))
			dirty = m_DirtyCount.Get(uid);
		dirty++;
		m_DirtyCount.Set(uid, dirty);

		if (dirty >= TXBConstants.LEDGER_FLUSH_THRESHOLD)
			Flush(uid);
	}

	static void Flush(string uid)
	{
		if (!m_Cache.Contains(uid)) return;
		ref TXBLedgerFile file = m_Cache.Get(uid);
		if (!file) return;

		if (!FileExist(TXBConstants.LEDGER_DIR))
			MakeDirectory(TXBConstants.LEDGER_DIR);

		string filePath = GetLedgerFilePath(uid);
		JsonFileLoader<TXBLedgerFile>.JsonSaveFile(filePath, file);
		m_DirtyCount.Set(uid, 0);
	}

	static void FlushAll()
	{
		array<string> keys = m_Cache.GetKeyArray();
		foreach (string uid : keys)
		{
			if (m_DirtyCount.Contains(uid) && m_DirtyCount.Get(uid) > 0)
				Flush(uid);
		}
	}

	// Tick from TXBBankingService.OnUpdate - flushes all dirty ledgers on interval.
	static void Tick(float timeslice)
	{
		m_FlushAccumulator += timeslice;
		if (m_FlushAccumulator < TXBConstants.LEDGER_FLUSH_INTERVAL) return;
		m_FlushAccumulator = 0;
		FlushAll();
	}

	// Filter helpers (client-side use on synced entries).
	static ref array<ref TXBLedgerEntry> FilterByType(array<ref TXBLedgerEntry> all, int type)
	{
		array<ref TXBLedgerEntry> outArray = new array<ref TXBLedgerEntry>();
		if (!all) return outArray;
		foreach (TXBLedgerEntry e : all)
		{
			if (e && e.type == type)
				outArray.Insert(e);
		}
		return outArray;
	}

	static int SumAmountSince(array<ref TXBLedgerEntry> all, int sinceEpoch, bool inflowsOnly, bool outflowsOnly)
	{
		int total = 0;
		if (!all) return 0;
		foreach (TXBLedgerEntry e : all)
		{
			if (!e || e.timestampUtc < sinceEpoch) continue;
			bool isIn = TXBLedgerEntryType.IsInflow(e.type);
			if (inflowsOnly && !isIn) continue;
			if (outflowsOnly && isIn) continue;
			total += e.amount;
		}
		return total;
	}
}
