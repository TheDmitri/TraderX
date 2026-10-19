// TraderXBanking - Ledger Tab Component
// DayZular component backing LedgerTabView.layout.
// Requests the server ledger on init, binds entries + selected-entry detail.

class TXBLedgerTabComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("txb-ledger-tab", TXBLedgerTabComponent);

	// ForEach binding - full visible entries (after filter)
	ref array<ref TXBLedgerEntry> _visibleEntries;
	// Empty-state text - blanked when there are visible entries (so it doesn't overlap the list).
	string emptyLedgerText;

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

	// Filter pill state - keyed by TXBLedgerEntryType constants, -1 = all
	int active_filter;

	// Summary strings (30 days)
	string stat_deposits;
	string stat_withdrawals;
	string stat_transfers;

	// Search box (two-way bound) + widgets whose visibility is driven here.
	string ledgerSearch;
	Widget ledgerSearchHint;
	Widget detailPanel;
	Widget detailEmpty;

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
		GetTXBLog().LogDebug("[TXB] LedgerTabComponent OnInit");

		_visibleEntries = new array<ref TXBLedgerEntry>();
		m_AllEntries = new array<ref TXBLedgerEntry>();
		active_filter = -1;
		selected_visible = false;
		UpdateDetailVisibility();

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

	void OnSearchChanged()
	{
		if (ledgerSearchHint)
			ledgerSearchHint.Show(ledgerSearch == "");
		ApplyFilter();
		MarkForCheck();
	}

	protected bool MatchesSearch(TXBLedgerEntry e, string needle)
	{
		if (needle == "")
			return true;
		string hay = e.counterpartyName + " " + e.note + " " + TXBLedgerEntryType.GetLabel(e.type) + " " + e.location;
		hay.ToLower();
		return hay.Contains(needle);
	}

	protected void ApplyFilter()
	{
		_visibleEntries.Clear();
		emptyLedgerText = "No transactions yet";
		if (!m_AllEntries) return;

		string needle = ledgerSearch;
		needle.TrimInPlace();
		needle.ToLower();
		if (needle != "")
			emptyLedgerText = "No transaction matches your search";

		foreach (TXBLedgerEntry e : m_AllEntries)
		{
			if (!e) continue;
			if (!MatchesSearch(e, needle)) continue;

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

		if (_visibleEntries.Count() > 0)
			emptyLedgerText = "";

		// Keep the detail panel meaningful: the selection must be one of the visible rows,
		// else show the most recent visible one (or the "select a transaction" state).
		bool selectionVisible = false;
		foreach (TXBLedgerEntry v : _visibleEntries)
		{
			if (v && v.id == selected_id)
				selectionVisible = true;
		}
		if (!selectionVisible)
		{
			selected_visible = false;
			selected_id = "";
			if (_visibleEntries.Count() > 0)
				SelectEntry(_visibleEntries[0].id);
		}
		UpdateDetailVisibility();
	}

	protected void UpdateDetailVisibility()
	{
		if (detailPanel) detailPanel.Show(selected_visible);
		if (detailEmpty) detailEmpty.Show(!selected_visible);
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

			bool inflow = TXBLedgerEntryType.IsInflow(e.type);

			selected_id = e.id;
			selected_date = FormatStampReadable(e.timestampUtc);
			selected_type = TXBLedgerEntryType.GetLabel(e.type);
			selected_counterparty = e.counterpartyName;
			selected_amount = TernaryString(inflow, "+", "-") + e.amount.ToString();
			selected_fee = e.fee.ToString();
			selected_balance = e.balanceAfter.ToString();
			selected_location = e.location;
			selected_note = e.note;
			selected_visible = true;
			UpdateDetailVisibility();
			MarkForCheck();
			ApplyDetailAmountColor(inflow);
			return;
		}
	}

	// Color the detail amount green for inflow (+) / red for outflow (-).
	protected void ApplyDetailAmountColor(bool inflow)
	{
		Widget root = GetRoot();
		if (!root) return;
		TextWidget aw = TextWidget.Cast(root.FindAnyWidget("detailAmount"));
		if (!aw) return;

		if (inflow)
			aw.SetColor(ARGB(255, 154, 176, 90));   // 9AB05A green
		else
			aw.SetColor(ARGB(255, 200, 102, 74));    // C8664A red
	}

	protected string FormatStampReadable(int stamp)
	{
		return TXBTimeUtil.FormatDateTime(stamp) + " UTC";
	}

	protected string Pad2(int v)
	{
		if (v < 10) return "0" + v.ToString();
		return v.ToString();
	}
}
