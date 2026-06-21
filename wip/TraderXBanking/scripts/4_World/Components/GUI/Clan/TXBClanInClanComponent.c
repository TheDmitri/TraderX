// TraderXBanking - Clan In-Clan Component
// Full in-clan UI: banner, sub-tabs, roster, invite, clan bank, log.
// Sub-tabs (Overview / Bank / Activity / Settings) toggle widget groups via Show().

class TXBClanInClanComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("banking-clan-inclan", TXBClanInClanComponent);

	// Banner
	string clanNameText;
	string clanMottoText;
	string clanFoundedText;
	string clanMembersCountText;
	string clanTagText;
	string clanRoleText;

	// KPI row
	string clanBankBalanceText;
	string clanVaultCapacityText;
	string clanOwnContribText;
	string clanOwnContribDetailText;

	// Roster
	string clanRosterTitleText;
	string rosterSearch;
	ref array<ref TXBClanMember> _members;

	// Invite panel
	string inviteRecipientId;
	string pendingInvitesTitleText;
	ref array<ref TXBClanInvite> _pendingOutgoingInvites;

	// Clan bank
	string clanBankDepositAmount;
	string clanBankWithdrawAmount;

	// Activity log (Overview shows recent subset; sub-tab Activity shows full)
	ref array<ref TXBClanActivityEntry> _clanActivity;

	// Internal
	protected int m_ViewerRole;  // TXBClanRole enum for the current player
	protected string m_CurrentSubview = "overview"; // overview | bank | activity | settings

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "banking-clan-inclan";
		m_Metadata.templatePath = "TraderXBanking/datasets/layouts/ClanInClanView.layout";
	}

	override void OnInit()
	{
		_members = new array<ref TXBClanMember>();
		_pendingOutgoingInvites = new array<ref TXBClanInvite>();
		_clanActivity = new array<ref TXBClanActivityEntry>();

		// TODO: hydrate from TXBClanService client cache, subscribe to events
		// TXBClanService.Event_OnClanUpdated.Insert(OnClanUpdated);
		// TXBClanService.Event_OnMembersUpdated.Insert(OnMembersUpdated);
		// TXBClanService.Event_OnInvitesUpdated.Insert(OnInvitesUpdated);
		// TXBClanService.Event_OnClanActivityUpdated.Insert(OnActivityUpdated);
		// TXBClanMemberRowComponent.Event_OnActionClicked.Insert(OnMemberAction);

		MarkForCheck();
	}

	override void OnDestroy()
	{
		// TODO: unsubscribe
	}

	// --- Sub-tab navigation ---

	void OnSubTabOverview() { SwitchSubview("overview"); }
	void OnSubTabBank()     { SwitchSubview("bank"); }
	void OnSubTabActivity() { SwitchSubview("activity"); }
	void OnSubTabSettings() { SwitchSubview("settings"); }

	protected void SwitchSubview(string name)
	{
		m_CurrentSubview = name;
		// TODO: toggle widget group visibility via GetRoot().FindAnyWidget(...).Show()
		// Also update the visual active state on the sub-tab buttons (text color).
		MarkForCheck();
	}

	// --- Actions ---

	void OnLeaveClan()         {} // TODO confirmation modal
	void OnSendInvite()        {} // TODO dispatch RPC
	void OnClanBankDeposit()   {} // TODO move money personal -> clan
	void OnClanBankWithdraw()  {} // TODO role-gated, move money clan -> personal
	void OnOpenFullLog()       {} // TODO switch to activity subtab
	void OnRosterSearchChanged() {} // TODO filter _members

	// Called from TXBClanMemberRowComponent.Event_OnActionClicked with (memberUid, action)
	void OnMemberAction(string memberUid, string action)
	{
		// action = "promote" | "demote" | "kick"
		// TODO dispatch appropriate RPC
	}
}
