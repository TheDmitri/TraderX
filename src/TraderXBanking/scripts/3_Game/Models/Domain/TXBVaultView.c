// TraderXBanking - Vault wire DTO (client view)
// Lean payload sent server>client as an array of ref objects (DayZular RPC), NOT a JSON string.
// The full TXBVaultData (with recursive snapshotData) is server-only; the client only needs
// display fields. Sending the heavy model as a JSON string overflowed the RPC string limit.

class TXBVaultItemDTO
{
	string id;
	string className;
	string displayName;
	int condition;        // health level 0-4
	int quantity;
	int footprintW;
	int footprintH;
	bool hasAttachments;  // true if the stored item has attachments or cargo
	string depositorName;
	int depositedUtc;

	void TXBVaultItemDTO()
	{
		id = "";
		className = "";
		displayName = "";
		condition = 0;
		quantity = 1;
		footprintW = 1;
		footprintH = 1;
		hasAttachments = false;
		depositorName = "";
		depositedUtc = 0;
	}
}

class TXBVaultView
{
	string ownerKey;
	int capacity;
	int slotsUsed;
	int feePerSlotPerDay;
	ref array<ref TXBVaultItemDTO> items;
	ref array<ref TXBVaultActivityEntry> activity;

	void TXBVaultView()
	{
		ownerKey = "";
		capacity = 0;
		slotsUsed = 0;
		feePerSlotPerDay = 0;
		items = new array<ref TXBVaultItemDTO>();
		activity = new array<ref TXBVaultActivityEntry>();
	}
}
