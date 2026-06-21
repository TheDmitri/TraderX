// TraderXBanking - Clan Tab Wrapper Component
// Top-level component mounted on the Clan shell tab. Decides which sub-view to mount
// based on the player's current clan membership.

class TXBClanTabComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("banking-clan-tab", TXBClanTabComponent);

	// Drives ComponentWidgetBinding on clanSubView panel.
	// Either "banking-clan-inclan" or "banking-clan-create".
	string current_clan_view = "banking-clan-create";

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "banking-clan-tab";
		m_Metadata.templatePath = "TraderXBanking/datasets/layouts/ClanTabView.layout";
	}

	override void OnInit()
	{
		RefreshView();

		// TODO: subscribe to clan membership events to switch view on the fly.
		// TXBClanService.Event_OnClanMembershipChanged.Insert(OnMembershipChanged);

		MarkForCheck();
	}

	override void OnDestroy()
	{
		// TODO: unsubscribe
	}

	protected void RefreshView()
	{
		// TODO: read player membership from TXBClanService.
		// For now default to create view. Switch to "banking-clan-inclan" when member.
		bool inClan = false;

		string target;
		if (inClan)
			target = "banking-clan-inclan";
		else
			target = "banking-clan-create";

		if (current_clan_view != target)
			current_clan_view = target;
	}

	void OnMembershipChanged()
	{
		RefreshView();
		MarkForCheck();
	}
}
