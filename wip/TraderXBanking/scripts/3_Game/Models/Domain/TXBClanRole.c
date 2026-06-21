// TraderXBanking - Clan Role Enum
// Role hierarchy: Member < Officer < Owner. Stored as int for JSON compactness.

class TXBClanRole
{
	static const int MEMBER  = 0;
	static const int OFFICER = 1;
	static const int OWNER   = 2;

	static string GetLabel(int r)
	{
		switch (r)
		{
			case OWNER:   return "Owner";
			case OFFICER: return "Officer";
			case MEMBER:  return "Member";
		}
		return "Unknown";
	}

	static bool CanInvite(int r)       { return r >= OFFICER; }
	static bool CanKickMember(int r)   { return r >= OFFICER; }
	static bool CanPromote(int r)      { return r == OWNER; }
	static bool CanWithdrawBank(int r) { return r >= OFFICER; }
	static bool CanWithdrawVault(int r){ return r >= OFFICER; }
	static bool CanManageSettings(int r){ return r == OWNER; }
}
