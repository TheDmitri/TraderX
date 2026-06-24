// TraderXBanking - Ledger Row Component
// ForEach row template for the ledger table. One instance per TXBLedgerEntry.
// Clicking the row fires Event_OnLedgerRowClicked with the entry id - consumed
// by TXBLedgerTabComponent to populate the detail panel.

class TXBLedgerRowComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("txb-ledger-row", TXBLedgerRowComponent);

	// Global event - parent tab subscribes to this.
	static ref ScriptInvoker Event_OnLedgerRowClicked = new ScriptInvoker();

	// Input (bound by ForEach via Component_ItemProperty)
	ref TXBLedgerEntry _inputEntry;

	// Display bindings
	string rowDateText;
	string rowTypeText;
	string rowAmountText;
	string rowBalanceText;

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "txb-ledger-row";
		m_Metadata.templatePath = "TraderXBanking/datasets/layouts/LedgerRowView.layout";
	}

	override void OnInit()
	{
		if (!_inputEntry)
		{
			rowDateText = "-";
			rowTypeText = "-";
			rowAmountText = "-";
			rowBalanceText = "-";
			MarkForCheck();
			return;
		}

		rowDateText = FormatDate(_inputEntry.timestampUtc);
		rowTypeText = BuildTypeLabel(_inputEntry);
		rowAmountText = BuildAmountLabel(_inputEntry);
		rowBalanceText = _inputEntry.balanceAfter.ToString();

		MarkForCheck();
		ApplyAmountColor();
	}

	// Color the amount green for inflow (+) / red for outflow (-) - text color is set in
	// code because the DZML dz-c-* class is static.
	protected void ApplyAmountColor()
	{
		Widget root = GetRoot();
		if (!root) return;
		TextWidget aw = TextWidget.Cast(root.FindAnyWidget("rowAmount"));
		if (!aw) return;

		if (_inputEntry && TXBLedgerEntryType.IsInflow(_inputEntry.type))
			aw.SetColor(ARGB(255, 154, 176, 90));   // 9AB05A olive/green
		else
			aw.SetColor(ARGB(255, 200, 102, 74));    // C8664A rust/red
	}

	void OnRowClicked()
	{
		if (!_inputEntry) return;
		Event_OnLedgerRowClicked.Invoke(_inputEntry.id);
	}

	// "Deposit" or "Transfer out - PlayerName"
	protected string BuildTypeLabel(TXBLedgerEntry e)
	{
		string label = TXBLedgerEntryType.GetLabel(e.type);
		if (e.counterpartyName != "" && e.type != TXBLedgerEntryType.DEPOSIT && e.type != TXBLedgerEntryType.WITHDRAW)
			return label + " - " + e.counterpartyName;
		return label;
	}

	// "+1500" for inflow, "-1500" for outflow.
	protected string BuildAmountLabel(TXBLedgerEntry e)
	{
		if (TXBLedgerEntryType.IsInflow(e.type))
			return "+" + e.amount.ToString();
		return "-" + e.amount.ToString();
	}

	// Epoch seconds -> "MM-DD HH:MM"
	protected string FormatDate(int stamp)
	{
		return TXBTimeUtil.FormatShort(stamp);
	}

	protected string Pad2(int v)
	{
		if (v < 10) return "0" + v.ToString();
		return v.ToString();
	}
}
