// TraderXBanking - Vault Item Row Component
// ForEach row for the vault storage grid. One instance per TXBVaultItem.
// Clicking the slot fires Event_OnItemClicked with the item id.

class TXBVaultItemRowComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register =
		new ComponentAutoRegister("txb-vault-item-row", TXBVaultItemRowComponent);

	static ref ScriptInvoker Event_OnItemClicked = new ScriptInvoker();

	// Input (ForEach item property)
	ref TXBVaultItem _inputItem;

	// Display bindings
	string displayName;
	string qtyText;
	string depositorInitials;
	string footprintText;
	string healthStyleRef;

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "txb-vault-item-row";
		m_Metadata.templatePath = "TraderXBanking/datasets/layouts/VaultItemRowView.layout";
	}

	override void OnInit()
	{
		if (!_inputItem)
		{
			displayName = "—";
			qtyText = "";
			depositorInitials = "";
			footprintText = "";
			healthStyleRef = "TXSurface";
			MarkForCheck();
			return;
		}

		displayName = _inputItem.displayName;
		qtyText = FormatQty(_inputItem.quantity);
		depositorInitials = FormatInitials(_inputItem.depositorName);
		footprintText = _inputItem.footprintW.ToString() + "x" + _inputItem.footprintH.ToString();
		healthStyleRef = ResolveHealthStyle(_inputItem.healthLevel);

		MarkForCheck();
	}

	void OnSlotClicked()
	{
		if (!_inputItem) return;
		Event_OnItemClicked.Invoke(_inputItem.id);
	}

	protected string FormatQty(int q)
	{
		if (q <= 1) return "";
		return "x" + q.ToString();
	}

	protected string FormatInitials(string name)
	{
		if (name == "" || name.Length() < 2) return "";

		string initials = name.Substring(0, 2);
		initials.ToUpper();
		return initials;
	}

	// Returns the style name to apply on the health dot (TXProgressFill/Warning/Danger).
	protected string ResolveHealthStyle(int level)
	{
		if (level <= 1) return "TXProgressFill";   // pristine -> olive
		if (level == 2) return "TXWarningBox";     // worn -> amber
		if (level == 3) return "TXBtnDanger";      // damaged -> rust
		return "TXErrorBox";                        // badly/ruined
	}
}
