// TraderXBanking - Clan Invite Model
// Pending invitation record. Persisted until accepted, declined, revoked, or expired.

class TXBClanInvite
{
	static const int DIRECTION_INCOMING = 0;  // you were invited
	static const int DIRECTION_OUTGOING = 1;  // you (an officer) invited someone

	string id;
	string clanId;
	string clanName;
	string clanTag;
	string senderUid;         // the officer who sent the invite
	string senderName;
	string recipientUid;      // the player being invited
	int createdUtc;
	int expiresUtc;
	int direction;            // set by the component when building UI rows

	// UI helpers
	string metaText;          // e.g. "Invited by Nikolai K. · 6 members · expires in 2d"

	void TXBClanInvite()
	{
		id = "";
		clanId = "";
		clanName = "";
		clanTag = "";
		senderUid = "";
		senderName = "";
		recipientUid = "";
		createdUtc = 0;
		expiresUtc = 0;
		direction = DIRECTION_INCOMING;
		metaText = "";
	}
}
