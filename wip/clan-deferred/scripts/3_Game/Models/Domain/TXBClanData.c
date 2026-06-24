// TraderXBanking - Clan Data Models
// Clan record + membership + activity log. JSON-serialisable, persisted per clan id.

class TXBClanMember
{
	string uid;
	string name;
	int role;               // TXBClanRole
	int joinedUtc;          // YYYYMMDDhhmmss
	string joinedDate;      // human-readable slice (for UI only; recomputed on sync)
	int contribution30d;    // roubles-in over last 30 days

	// UI-only helpers populated by TXBClanInClanComponent when building rows.
	// Reflect the viewer's rights against this row (not persisted).
	bool canPromote;
	bool canKick;

	void TXBClanMember()
	{
		uid = "";
		name = "";
		role = TXBClanRole.MEMBER;
		joinedUtc = 0;
		joinedDate = "";
		contribution30d = 0;
		canPromote = false;
		canKick = false;
	}
}

class TXBClanActivityEntry
{
	string id;
	int timestampUtc;
	string actorUid;
	string actorName;
	string description;     // human-readable event, e.g. "deposited SVD to vault"
	string deltaText;       // e.g. "+item", "−312 ₽", "+member"
	int activityType;       // 0=bank_in, 1=bank_out, 2=vault_in, 3=vault_out, 4=member_join, 5=member_kick, 6=role_change, 7=fee

	void TXBClanActivityEntry()
	{
		id = "";
		timestampUtc = 0;
		actorUid = "";
		actorName = "";
		description = "";
		deltaText = "";
		activityType = 0;
	}
}

class TXBClanData
{
	string version;
	string clanId;
	string name;
	string tag;
	string motto;
	int foundedUtc;
	int maxMembers;
	string ownerUid;
	int bankBalance;
	int vaultCapacity;
	int vaultUsed;
	ref array<ref TXBClanMember> members;
	ref array<ref TXBClanActivityEntry> activity;

	void TXBClanData()
	{
		version = "1.0";
		clanId = "";
		name = "";
		tag = "";
		motto = "";
		foundedUtc = 0;
		maxMembers = 20;
		ownerUid = "";
		bankBalance = 0;
		vaultCapacity = 0;
		vaultUsed = 0;
		members = new array<ref TXBClanMember>();
		activity = new array<ref TXBClanActivityEntry>();
	}
}
