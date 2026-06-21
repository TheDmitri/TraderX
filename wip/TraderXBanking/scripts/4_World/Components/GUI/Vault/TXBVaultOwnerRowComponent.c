// TraderXBanking - Vault Owner Row Component
// Dropdown option row in the vault owner switcher.
// Click fires Event_OnOwnerSelected with the owner key ("player:UID" or "clan:ID").

class TXBVaultOwnerRowComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("txb-vault-owner-row", TXBVaultOwnerRowComponent);

	static ref ScriptInvoker Event_OnOwnerSelected = new ScriptInvoker();

	ref TXBVaultOwner _inputOwner;

	string ownerName;
	string ownerSub;
	string ownerBadge;

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "txb-vault-owner-row";
		m_Metadata.templatePath = "TraderXBanking/datasets/layouts/VaultOwnerRowView.layout";
	}

	override void OnInit()
	{
		if (!_inputOwner)
		{
			ownerName = "—";
			ownerSub = "";
			ownerBadge = "";
			MarkForCheck();
			return;
		}

		ownerName = _inputOwner.displayName;
		ownerSub = _inputOwner.capacitySummary;
		ownerBadge = _inputOwner.roleBadge;
		MarkForCheck();
	}

	void OnOwnerRowClicked()
	{
		if (!_inputOwner) return;
		Event_OnOwnerSelected.Invoke(_inputOwner.ownerKey);
	}
}
