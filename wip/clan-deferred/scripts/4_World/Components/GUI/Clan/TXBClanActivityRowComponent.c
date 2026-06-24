// TraderXBanking - Clan Activity Row Component
// ForEach row for clan/vault activity log. Used by ClanInClanView, VaultTabView, HomeTabView.

class TXBClanActivityRowComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("txb-clan-activity-row", TXBClanActivityRowComponent);

	ref TXBClanActivityEntry _inputActivity;

	string activityDateText;
	string activityWhoText;
	string activityDescText;
	string activityDeltaText;

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "txb-clan-activity-row";
		m_Metadata.templatePath = "TraderXBanking/datasets/layouts/ClanActivityRowView.layout";
	}

	override void OnInit()
	{
		if (!_inputActivity)
		{
			activityDateText = "—";
			activityWhoText = "—";
			activityDescText = "—";
			activityDeltaText = "—";
			MarkForCheck();
			return;
		}

		activityDateText = FormatDate(_inputActivity.timestampUtc);
		activityWhoText = _inputActivity.actorName;
		activityDescText = _inputActivity.description;
		activityDeltaText = _inputActivity.deltaText;

		MarkForCheck();
	}

	// YYYYMMDDhhmmss int -> "MM-DD HH:MM"
	protected string FormatDate(int stamp)
	{
		int mn = (stamp / 100) % 100;
		int h = (stamp / 10000) % 100;
		int d = (stamp / 1000000) % 100;
		int mo = (stamp / 100000000) % 100;
		return Pad2(mo) + "-" + Pad2(d) + " " + Pad2(h) + ":" + Pad2(mn);
	}

	protected string Pad2(int v)
	{
		if (v < 10) return "0" + v.ToString();
		return v.ToString();
	}
}
