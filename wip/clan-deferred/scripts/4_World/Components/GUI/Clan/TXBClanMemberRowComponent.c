// TraderXBanking - Clan Member Row Component
// ForEach row for the clan roster. One instance per TXBClanMember.
// Action buttons fire Event_OnActionClicked with (memberUid, "promote"|"demote"|"kick").

class TXBClanMemberRowComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("txb-clan-member-row", TXBClanMemberRowComponent);

	static ref ScriptInvoker Event_OnActionClicked = new ScriptInvoker();

	ref TXBClanMember _inputMember;

	string avatarInitials;
	string memberNameText;
	string memberSubText;
	string memberContribText;
	string memberRoleText;

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "txb-clan-member-row";
		m_Metadata.templatePath = "TraderXBanking/datasets/layouts/ClanMemberRowView.layout";
	}

	override void OnInit()
	{
		if (!_inputMember)
		{
			avatarInitials = "—";
			memberNameText = "—";
			memberSubText = "";
			memberContribText = "+0";
			memberRoleText = "";
			MarkForCheck();
			return;
		}

		avatarInitials = FormatInitials(_inputMember.name);
		memberNameText = _inputMember.name;
		memberSubText = _inputMember.uid + " · joined " + _inputMember.joinedDate;
		memberContribText = "+" + _inputMember.contribution30d.ToString();
		memberRoleText = TXBClanRole.GetLabel(_inputMember.role);

		ApplyActionVisibility();
		MarkForCheck();
	}

	void OnPromoteClicked()
	{
		if (!_inputMember) return;
		// If member is Officer, a promote press acts as a demote intent in the component above.
		// Here we fire the raw "promote" event; consumer handles rank arithmetic.
		Event_OnActionClicked.Invoke(_inputMember.uid, "promote");
	}

	void OnKickClicked()
	{
		if (!_inputMember) return;
		Event_OnActionClicked.Invoke(_inputMember.uid, "kick");
	}

	// Hides promote/kick for self-row and for rows where the viewer lacks rights.
	// Rights matrix is read from the member object (prepared by TXBClanInClanComponent).
	protected void ApplyActionVisibility()
	{
		if (!_inputMember) return;
		Widget root = GetRoot();
		if (!root) return;

		Widget promoteBtn = root.FindAnyWidget("memberPromoteBtn");
		Widget kickBtn = root.FindAnyWidget("memberKickBtn");

		if (promoteBtn) promoteBtn.Show(_inputMember.canPromote);
		if (kickBtn)    kickBtn.Show(_inputMember.canKick);

		// Owner-role rows get a different promote label ("Demote" when already Officer).
		// TODO: update button text via SetText when framework allows.
	}

	protected string FormatInitials(string name)
	{
		if (name == "" || name.Length() < 2) return "—";
		string initials = name.Substring(0, 2);
		initials.ToUpper();
		return initials;
	}
}
