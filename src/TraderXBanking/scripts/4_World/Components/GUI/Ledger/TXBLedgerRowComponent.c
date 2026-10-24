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
	// Nasdara row: "+" / "−" box and "10/23 14:02 · balance 500" under the type
	string rowSignText;
	string rowDetailText;

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
		rowSignText = "−";
		if (TXBLedgerEntryType.IsInflow(_inputEntry.type)) rowSignText = "+";
		rowDetailText = rowDateText + " · " + Widget.TranslateString("#txb_balance_after") + " " + TXBFormat.Currency(_inputEntry.balanceAfter);

		MarkForCheck();
		ApplyAmountColor();
	}

	// Amount and sign box in Good #7FA65A for inflow, Bad #E07A63 for outflow - text color
	// is set in code because the DZML dz-c-* class is static.
	protected void ApplyAmountColor()
	{
		Widget root = GetRoot();
		if (!root) return;
		int color = ARGB(255, 224, 122, 99);
		if (_inputEntry && TXBLedgerEntryType.IsInflow(_inputEntry.type))
			color = ARGB(255, 127, 166, 90);
		TextWidget aw = TextWidget.Cast(root.FindAnyWidget("rowAmount"));
		if (aw) aw.SetColor(color);
		TextWidget sw = TextWidget.Cast(root.FindAnyWidget("rowSign"));
		if (sw) sw.SetColor(color);
	}

	void OnRowClicked()
	{
		if (!_inputEntry) return;
		Event_OnLedgerRowClicked.Invoke(_inputEntry.id);
	}

	// "Deposit" or "Transfer out - PlayerName (+5 fee)"
	protected string BuildTypeLabel(TXBLedgerEntry e)
	{
		string label = TXBLedgerEntryType.GetLabel(e.type);
		if (e.counterpartyName != "" && e.type != TXBLedgerEntryType.DEPOSIT && e.type != TXBLedgerEntryType.WITHDRAW)
			label = label + " - " + e.counterpartyName;
		// Transfer fee is charged on top of the sent amount: make it visible in the row.
		// Deposit fee is paid on top of the deposited amount (amount = credited).
		if (e.type == TXBLedgerEntryType.DEPOSIT && e.fee > 0)
			label = label + " (fee " + e.fee.ToString() + ")";
		if (e.type == TXBLedgerEntryType.TRANSFER_OUT && e.fee > 0)
			label = label + " (+" + e.fee.ToString() + " fee)";
		return label;
	}

	// "+1500" for inflow, "-1500" for outflow.
	protected string BuildAmountLabel(TXBLedgerEntry e)
	{
		if (TXBLedgerEntryType.IsInflow(e.type))
			return "+" + TXBFormat.Currency(e.amount);
		return "−" + TXBFormat.Currency(e.amount);
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
