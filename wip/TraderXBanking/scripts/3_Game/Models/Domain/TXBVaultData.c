// TraderXBanking - Vault Data Models
// Polymorphic vault (player-owned OR clan-owned). Snapshots item state on deposit
// so withdraw can recreate the item with the same health / qty / attachments.

class TXBVaultOwner
{
	// ownerKey = "player:<UID>" or "clan:<clanId>"
	string ownerKey;
	string ownerType;       // "player" | "clan"
	string ownerId;
	string displayName;     // "Personal safe · Box #47" or "Clan · \"Chernyi Led\""
	string capacitySummary; // "42 / 100 slots · −126 ₽/day"
	string roleBadge;       // "Solo" | "Member" | "Officer" | "Owner"

	void TXBVaultOwner()
	{
		ownerKey = "";
		ownerType = "";
		ownerId = "";
		displayName = "";
		capacitySummary = "";
		roleBadge = "";
	}
}

// Full recursive entity state — mirrors CarPackingItemData pattern.
// Stored inside TXBVaultItem.snapshotData so the vault can faithfully recreate
// health, quantity, temperature, ammo, attachments and cargo on withdrawal.
class TXBVaultItemData
{
	string className;
	int slot;
	int locType;    // InventoryLocationType cast to int for JSON
	int locIdx;
	int locRow;
	int locCol;
	bool locFlip;
	float health;   // normalized 0-1 (GetHealth01)
	float quantity;
	int liquidType;
	float temp;
	float wetness;
	float energy;
	int magAmmoCount;
	ref array<ref TXBVaultItemData> children;

	void TXBVaultItemData()
	{
		className = "";
		slot = -1;
		locType = 0;
		locIdx = 0;
		locRow = 0;
		locCol = 0;
		locFlip = false;
		health = 1.0;
		quantity = 0;
		liquidType = 0;
		temp = 0;
		wetness = 0;
		energy = 0;
		magAmmoCount = 0;
		children = new array<ref TXBVaultItemData>();
	}
}

class TXBVaultItem
{
	string id;
	string className;
	string displayName;
	int quantity;           // stacks / magazine count / ammo round count
	int healthLevel;        // 0=pristine, 1=worn, 2=damaged, 3=badly, 4=ruined
	int footprintW;
	int footprintH;
	string depositorUid;
	string depositorName;
	int depositedUtc;
	int daysStored;
	int accruedFee;
	string attachmentsSummary; // e.g. "PSO-1 · Suppressor"
	string cargoSummary;       // e.g. "30 rnd loaded"
	bool ownerLocked;          // clan-only; if true, only Owner + depositor can withdraw
	ref TXBVaultItemData snapshotData; // full recursive entity state for faithful recreation
	EntityAI entityRef;        // client-side only: live entity ref for inventory pseudo-items

	void TXBVaultItem()
	{
		id = "";
		className = "";
		displayName = "";
		quantity = 1;
		healthLevel = 0;
		footprintW = 1;
		footprintH = 1;
		depositorUid = "";
		depositorName = "";
		depositedUtc = 0;
		daysStored = 0;
		accruedFee = 0;
		attachmentsSummary = "";
		cargoSummary = "";
		ownerLocked = false;
		snapshotData = null;
		entityRef = null;
	}
}

class TXBVaultActivityEntry
{
	// Shares the shape of TXBClanActivityEntry so the same row component renders both.
	// Kept as a distinct type so vault-level filtering (item-only events) stays clean.
	string id;
	int timestampUtc;
	string actorUid;
	string actorName;
	string description;
	string deltaText;
	int activityType;       // 0=deposit, 1=withdraw, 2=fee

	void TXBVaultActivityEntry()
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

class TXBVaultData
{
	string version;
	string ownerKey;
	string ownerType;
	string ownerId;
	int capacity;
	int slotsUsed;
	int feePerSlotPerDay;
	int lastAccrualUtc;
	ref array<ref TXBVaultItem> items;
	ref array<ref TXBVaultActivityEntry> activity;

	void TXBVaultData()
	{
		version = "1.0";
		ownerKey = "";
		ownerType = "";
		ownerId = "";
		capacity = 0;
		slotsUsed = 0;
		feePerSlotPerDay = 0;
		lastAccrualUtc = 0;
		items = new array<ref TXBVaultItem>();
		activity = new array<ref TXBVaultActivityEntry>();
	}

	// Called after JSON load to ensure arrays are initialized (JSON loader may leave them null)
	void InitAfterLoad()
	{
		if (!items) items = new array<ref TXBVaultItem>();
		if (!activity) activity = new array<ref TXBVaultActivityEntry>();
	}
}
