// TraderXBanking - Clan Invite Row Component
// ForEach row for pending invites. Used in three contexts:
//   - Home tab alerts column (incoming invites for the player)
//   - ClanCreateTab incoming invites list
//   - ClanInClanTab outgoing invites list (labels swap to "Revoke")
// Action clicks fire Event_OnInviteAction with (inviteId, "accept"|"decline"|"revoke").

class TXBClanInviteRowComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("txb-clan-invite-row", TXBClanInviteRowComponent);

	static ref ScriptInvoker Event_OnInviteAction = new ScriptInvoker();

	ref TXBClanInvite _inputInvite;

	string inviteCrestText;
	string inviteNameText;
	string inviteMetaText;
	string inviteAcceptLabel;
	string inviteDeclineLabel;

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "txb-clan-invite-row";
		m_Metadata.templatePath = "TraderXBanking/datasets/layouts/ClanInviteRowView.layout";
	}

	override void OnInit()
	{
		if (!_inputInvite)
		{
			inviteCrestText = "—";
			inviteNameText = "—";
			inviteMetaText = "";
			inviteAcceptLabel = "ACCEPT";
			inviteDeclineLabel = "DECLINE";
			MarkForCheck();
			return;
		}

		inviteCrestText = _inputInvite.clanTag;
		inviteNameText = _inputInvite.clanName;
		inviteMetaText = _inputInvite.metaText;

		// Labels swap depending on direction (outgoing invites = Revoke only)
		if (_inputInvite.direction == TXBClanInvite.DIRECTION_OUTGOING)
		{
			inviteAcceptLabel = "";
			inviteDeclineLabel = "REVOKE";
			HideAcceptButton();
		}
		else
		{
			inviteAcceptLabel = "ACCEPT";
			inviteDeclineLabel = "DECLINE";
		}

		MarkForCheck();
	}

	void OnAcceptClicked()
	{
		if (!_inputInvite) return;
		Event_OnInviteAction.Invoke(_inputInvite.id, "accept");
	}

	void OnDeclineClicked()
	{
		if (!_inputInvite) return;
		string action;
		if (_inputInvite.direction == TXBClanInvite.DIRECTION_OUTGOING)
			action = "revoke";
		else
			action = "decline";
		Event_OnInviteAction.Invoke(_inputInvite.id, action);
	}

	protected void HideAcceptButton()
	{
		Widget root = GetRoot();
		if (!root) return;
		Widget acceptBtn = root.FindAnyWidget("inviteAcceptBtn");
		if (acceptBtn) acceptBtn.Show(false);
		// Widen decline to fill the space
		Widget declineBtn = root.FindAnyWidget("inviteDeclineBtn");
		if (declineBtn) declineBtn.SetSize(0.30, 0.50);
	}
}
