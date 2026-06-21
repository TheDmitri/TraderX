// TraderXBanking - Ledger Tab Component
// DayZular component backing LedgerTabView.layout.
// Requests the server ledger on init, binds entries + selected-entry detail.

class TXBLedgerTabComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("txb-ledger-tab", TXBLedgerTabComponent);

	// ForEach binding — full visible entries (after filter)
	ref array<ref TXBLedgerEntry> _visibleEntries;

	// Selected entry drill-down (bindings read these)
	string selected_id;
	string selected_date;
	string selected_type;
	string selected_counterparty;
	string selected_amount;
	string selected_fee;
	string selected_balance;
	string selected_location;
	string selected_note;
	bool selected_visible;

	// Filter pill state — keyed by TXBLedgerEntryType constants, -1 = all
	int active_filter;

	// Summary strings (30 days)
	string stat_deposits;
	string stat_withdrawals;
	string stat_transfers;

	// Internal cache
	protected ref array<ref TXBLedgerEntry> m_AllEntries;

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "txb-ledger-tab";
		m_Metadata.templatePath = "TraderXBanking/datasets/layouts/LedgerTabView.layout";
	}

	override void OnInit()
	{
		DZLog().LogDebug("[TXB] LedgerTabComponent OnInit");

		_visibleEntries = new array<ref TXBLedgerEntry>();
		m_AllEntries = new array<ref TXBLedgerEntry>();
		active_filter = -1;
		selected_visible = false;

		TXBBankingService.Event_OnLedgerUpdated.Insert(OnLedgerUpdated);
		TXBLedgerRowComponent.Event_OnLedgerRowClicked.Insert(OnRowClicked);

		// Seed from any ledger already synced before this tab was mounted.
		OnLedgerUpdated();

		// Kick server to send ledger (100 latest entries).
		if (GetGame().IsClient() || !GetGame().IsMultiplayer())
		{
			GetDayZularRPCManager().SendRPC(TXBConstants.RPC_NAMESPACE, "RequestLedger", new Param1<int>(100), true, null);
		}

		MarkForCheck();
	}

	override void OnDestroy()
	{
		TXBBankingService.Event_OnLedgerUpdated.Remove(OnLedgerUpdated);
		TXBLedgerRowComponent.Event_OnLedgerRowClicked.Remove(OnRowClicked);
	}

	// Invoked when any ledger row is clicked (static event from TXBLedgerRowComponent).
	void OnRowClicked(string entryId)
	{
		SelectEntry(entryId);
	}

	// Called when server pushes a new entry OR when client cache is replaced after sync.
	void OnLedgerUpdated()
	{
		array<ref TXBLedgerEntry> cached = GetTXBBankingService().GetClientLedger();
		m_AllEntries.Clear();
		if (cached)
		{
			foreach (TXBLedgerEntry e : cached)
			{
				if (e) m_AllEntries.Insert(e);
			}
		}

		ApplyFilter();
		ComputeStats();
		MarkForCheck();
	}

	// --- Filter pills ---

	bool OnFilterAll()     { active_filter = -1; ApplyFilter(); MarkForCheck(); return true; }
	bool OnFilterDeposit() { active_filter = TXBLedgerEntryType.DEPOSIT; ApplyFilter(); MarkForCheck(); return true; }
	bool OnFilterWithdraw(){ active_filter = TXBLedgerEntryType.WITHDRAW; ApplyFilter(); MarkForCheck(); return true; }
	bool OnFilterTransfer(){ active_filter = TXBLedgerEntryType.TRANSFER_OUT; ApplyFilter(); MarkForCheck(); return true; }
	bool OnFilterTrader()  { active_filter = TXBLedgerEntryType.TRADER_BUY; ApplyFilter(); MarkForCheck(); return true; }

	protected void ApplyFilter()
	{
		_visibleEntries.Clear();
		if (!m_AllEntries) return;

		foreach (TXBLedgerEntry e : m_AllEntries)
		{
			if (!e) continue;

			if (active_filter < 0)
			{
				_visibleEntries.Insert(e);
				continue;
			}

			// Transfer filter matches both IN and OUT
			if (active_filter == TXBLedgerEntryType.TRANSFER_OUT)
			{
				if (e.type == TXBLedgerEntryType.TRANSFER_OUT || e.type == TXBLedgerEntryType.TRANSFER_IN)
					_visibleEntries.Insert(e);
				continue;
			}

			// Trader filter matches BUY and SELL
			if (active_filter == TXBLedgerEntryType.TRADER_BUY)
			{
				if (e.type == TXBLedgerEntryType.TRADER_BUY || e.type == TXBLedgerEntryType.TRADER_SELL)
					_visibleEntries.Insert(e);
				continue;
			}

			if (e.type == active_filter)
				_visibleEntries.Insert(e);
		}
	}

	// --- Stats (30-day rolling) ---

	protected void ComputeStats()
	{
		int deposits = 0;
		int withdrawals = 0;
		int transfers = 0;

		if (!m_AllEntries)
		{
			stat_deposits = "0";
			stat_withdrawals = "0";
			stat_transfers = "0";
			return;
		}

		foreach (TXBLedgerEntry e : m_AllEntries)
		{
			if (!e) continue;
			if (e.type == TXBLedgerEntryType.DEPOSIT) deposits = deposits + e.amount;
			else if (e.type == TXBLedgerEntryType.WITHDRAW) withdrawals = withdrawals + e.amount;
			else if (e.type == TXBLedgerEntryType.TRANSFER_OUT || e.type == TXBLedgerEntryType.TRANSFER_IN) transfers = transfers + 1;
		}

		stat_deposits = "+" + deposits.ToString();
		stat_withdrawals = "-" + withdrawals.ToString();
		stat_transfers = transfers.ToString();
	}

	// --- Selection ---

	void SelectEntry(string entryId)
	{
		if (!m_AllEntries) return;
		foreach (TXBLedgerEntry e : m_AllEntries)
		{
			if (!e || e.id != entryId) continue;

			selected_id = e.id;
			selected_date = FormatStampReadable(e.timestampUtc);
			selected_type = TXBLedgerEntryType.GetLabel(e.type);
			selected_counterparty = e.counterpartyName;
			selected_amount = e.amount.ToString();
			selected_fee = e.fee.ToString();
			selected_balance = e.balanceAfter.ToString();
			selected_location = e.location;
			selected_note = e.note;
			selected_visible = true;
			MarkForCheck();
			return;
		}
	}

	protected string FormatStampReadable(int stamp)
	{
		// stamp is YYYYMMDDhhmmss — slice into readable form.
		int s = stamp % 100;
		int mn = (stamp / 100) % 100;
		int h = (stamp / 10000) % 100;
		int d = (stamp / 1000000) % 100;
		int mo = (stamp / 100000000) % 100;
		int y = stamp / 10000000000;
		return y.ToString() + "-" + Pad2(mo) + "-" + Pad2(d) + " " + Pad2(h) + ":" + Pad2(mn) + ":" + Pad2(s) + " UTC";
	}

	protected string Pad2(int v)
	{
		if (v < 10) return "0" + v.ToString();
		return v.ToString();
	}
}
