// TraderXBanking - Clan Create / Join Component
// Displayed when the player has no clan affiliation.
// Provides the clan-founding form, incoming invites list, and join-by-code input.

class TXBClanCreateTabComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("banking-clan-create", TXBClanCreateTabComponent);

	// Create form
	string newClanName;
	string newClanTag;
	string newClanMaxMembers;
	string newClanMotto;
	string newClanInitialDeposit;

	// Cost preview (computed from settings.founderFee + newClanInitialDeposit)
	string founderFeeText;
	string createTotalCostText;
	string foundButtonLabel;

	// Incoming invites
	ref array<ref TXBClanInvite> _incomingInvites;

	// Join by code
	string inviteCode;

	// Internal
	protected int m_FounderFee;

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "banking-clan-create";
		m_Metadata.templatePath = "TraderXBanking/datasets/layouts/ClanCreateTabView.layout";
	}

	override void OnInit()
	{
		_incomingInvites = new array<ref TXBClanInvite>();

		newClanMaxMembers = "20";
		newClanInitialDeposit = "0";
		inviteCode = "";

		// TODO: read founder fee from TXBSettings (server-synced)
		// TXBSettings s = GetTXBConfigService().GetClientSettings();
		// if (s) m_FounderFee = s.clanFounderFee;
		m_FounderFee = 5000; // placeholder default

		RebuildCostPreview();

		// TODO: subscribe to invites event
		// TXBClanService.Event_OnIncomingInvitesUpdated.Insert(OnInvitesUpdated);
		// TXBClanInviteRowComponent.Event_OnInviteAction.Insert(OnInviteAction);

		MarkForCheck();
	}

	override void OnDestroy()
	{
		// TODO: unsubscribe
	}

	void OnCreateAmountChanged()
	{
		RebuildCostPreview();
		MarkForCheck();
	}

	protected void RebuildCostPreview()
	{
		int initial = newClanInitialDeposit.ToInt();
		int total = m_FounderFee + initial;
		founderFeeText = m_FounderFee.ToString() + " ₽";
		createTotalCostText = total.ToString() + " ₽";
		foundButtonLabel = "FOUND CLAN · " + total.ToString() + " ₽";
	}

	// --- Actions ---

	void OnFoundClan()
	{
		// TODO validation: name not empty, tag 3-5 letters, player has enough balance
		// Dispatch RPC RequestCreateClan(name, tag, maxMembers, motto, initialDeposit)
	}

	void OnJoinByCode()
	{
		// TODO dispatch RPC RequestJoinByCode(inviteCode)
	}

	// Called by TXBClanInviteRowComponent.Event_OnInviteAction with (inviteId, action)
	void OnInviteAction(string inviteId, string action)
	{
		// action = "accept" | "decline"
		// TODO dispatch RPC
	}
}
