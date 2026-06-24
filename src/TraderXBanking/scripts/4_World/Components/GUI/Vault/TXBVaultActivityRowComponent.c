// TraderXBanking - Vault Activity Row Component
// ForEach row for the vault activity log (deposit/withdraw/fee events).
// Binds TXBVaultActivityEntry. Neutral counterpart to the deferred clan activity row.

class TXBVaultActivityRowComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("txb-vault-activity-row", TXBVaultActivityRowComponent);

	ref TXBVaultActivityEntry _inputActivity;

	string activityDateText;
	string activityWhoText;
	string activityDescText;
	string activityDeltaText;

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "txb-vault-activity-row";
		m_Metadata.templatePath = "TraderXBanking/datasets/layouts/VaultActivityRowView.layout";
	}

	override void OnInit()
	{
		if (!_inputActivity)
		{
			activityDateText = "-";
			activityWhoText = "-";
			activityDescText = "-";
			activityDeltaText = "-";
			MarkForCheck();
			return;
		}

		activityDateText = FormatDate(_inputActivity.timestampUtc);
		activityWhoText = _inputActivity.actorName;
		activityDescText = _inputActivity.description;
		activityDeltaText = _inputActivity.deltaText;

		MarkForCheck();
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
