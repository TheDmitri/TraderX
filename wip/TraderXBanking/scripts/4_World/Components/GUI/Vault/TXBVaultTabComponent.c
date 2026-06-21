// TraderXBanking - Vault Tab Component
// Hosts Personal vault. Switches between vault-view (stored items) and
// inventory-view (player items available to deposit) via m_ViewMode.

class TXBVaultTabComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("banking-vault-tab", TXBVaultTabComponent);

	// Owner selector
	string ownerDisplayText;
	string ownerRoleText;
	ref array<ref TXBVaultOwner> _availableOwners;

	// Capacity tiles
	string capacityText;
	string capacitySubText;
	string dailyFeeText;
	string feeSourceText;
	string capacityRoleText;
	string capacityRoleRightsText;
	string nextCollectionText;

	// Storage grid (vault items OR inventory items depending on m_ViewMode)
	string storageTitleText;
	string storageSearch;
	string emptyStorageText;
	ref array<ref TXBVaultItem> _visibleItems;

	// Item preview (inventory mode only — EntityAI from player inventory)
	EntityAI previewItem;

	// Detail panel (selected vault item)
	string detailNameText;
	string detailHealthText;
	string detailClassText;
	string detailFootprintText;
	string detailSinceText;
	string detailDaysText;
	string detailAttachmentsText;
	string detailCargoText;
	string detailAccruedFeeText;
	string detailDepositorText;

	// Actions
	string actionsNoteText;

	// Activity log
	string emptyVaultActivityText;
	ref array<ref TXBVaultActivityEntry> _vaultActivity;

	// Internal
	protected string m_CurrentOwnerKey;
	protected string m_SelectedItemId;
	protected string m_SelectedInventoryItemId;
	protected bool m_DropdownOpen;
	protected string m_ViewMode;      // "vault" | "inventory"
	protected bool m_Throttled;
	protected ref array<ref TXBVaultItem> m_AllVaultItems;
	protected ref array<ref TXBVaultItem> m_InventoryItems;
	protected ref array<EntityAI> m_InventoryEntities;

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "banking-vault-tab";
		m_Metadata.templatePath = "TraderXBanking/datasets/layouts/VaultTabView.layout";
	}

	override void OnInit()
	{
		_availableOwners = new array<ref TXBVaultOwner>();
		_visibleItems = new array<ref TXBVaultItem>();
		_vaultActivity = new array<ref TXBVaultActivityEntry>();
		m_AllVaultItems = new array<ref TXBVaultItem>();
		m_InventoryItems = new array<ref TXBVaultItem>();
		m_InventoryEntities = new array<EntityAI>();

		m_ViewMode = "vault";
		m_CurrentOwnerKey = "player:" + GetLocalPlayerUID();
		m_SelectedInventoryItemId = "";

		emptyVaultActivityText = "No recent activity";
		actionsNoteText = "Withdraw spawns into inventory. Deposit snapshots health/qty/attachments.";
		previewItem = null;

		TXBVaultService.Event_OnVaultUpdated.Insert(OnVaultUpdated);
		TXBVaultItemCardComponent.Event_OnItemClicked.Insert(OnItemRowClicked);
		TXBVaultOwnerRowComponent.Event_OnOwnerSelected.Insert(SelectOwner);

		BuildOwnerList();
		HideDropdown();
		ClearDetailPanel();
		RequestVaultData();
		MarkForCheck();
	}

	override void OnDestroy()
	{
		TXBVaultService.Event_OnVaultUpdated.Remove(OnVaultUpdated);
		TXBVaultItemCardComponent.Event_OnItemClicked.Remove(OnItemRowClicked);
		TXBVaultOwnerRowComponent.Event_OnOwnerSelected.Remove(SelectOwner);
	}

	// --- Event handlers ---

	void OnVaultUpdated()
	{
		LoadFromClientCache();
		MarkForCheck();
	}

	// --- Owner list ---

	protected void BuildOwnerList()
	{
		_availableOwners.Clear();

		TXBVaultOwner personal = new TXBVaultOwner();
		personal.ownerKey = "player:" + GetLocalPlayerUID();
		personal.ownerType = "player";
		personal.ownerId = GetLocalPlayerUID();
		personal.displayName = "Personal Vault";
		personal.capacitySummary = "—";
		personal.roleBadge = "SOLO";
		_availableOwners.Insert(personal);

		ownerDisplayText = "Personal Vault";
		ownerRoleText = "SOLO";
	}

	// --- Owner switcher ---

	void OnSwitchOwner()
	{
		if (m_DropdownOpen) HideDropdown();
		else ShowDropdown();
	}

	protected void ShowDropdown()
	{
		m_DropdownOpen = true;
		Widget root = GetRoot();
		if (!root) return;
		Widget dropdown = root.FindAnyWidget("ownerDropdown");
		if (dropdown) dropdown.Show(true);
	}

	protected void HideDropdown()
	{
		m_DropdownOpen = false;
		Widget root = GetRoot();
		if (!root) return;
		Widget dropdown = root.FindAnyWidget("ownerDropdown");
		if (dropdown) dropdown.Show(false);
	}

	void SelectOwner(string ownerKey)
	{
		if (ownerKey == m_CurrentOwnerKey)
		{
			HideDropdown();
			return;
		}
		m_CurrentOwnerKey = ownerKey;
		HideDropdown();

		foreach (TXBVaultOwner vo : _availableOwners)
		{
			if (vo && vo.ownerKey == ownerKey)
			{
				ownerDisplayText = vo.displayName;
				ownerRoleText = vo.roleBadge;
				break;
			}
		}

		m_AllVaultItems.Clear();
		_visibleItems.Clear();
		_vaultActivity.Clear();
		ClearDetailPanel();
		RequestVaultData();
		MarkForCheck();
	}

	// --- Vault data request ---

	protected void RequestVaultData()
	{
		GetDayZularRPCManager().SendRPC(TXBConstants.RPC_NAMESPACE, "RequestVaultData", new Param1<string>(m_CurrentOwnerKey), true, null);
	}

	// --- Populate from client cache ---

	protected void LoadFromClientCache()
	{
		TXBVaultData vault = GetTXBVaultService().GetClientVault();
		if (!vault)
		{
			capacityText = "0 / 0";
			capacitySubText = "slots";
			dailyFeeText = "—";
			feeSourceText = "no active vault";
			capacityRoleText = "—";
			capacityRoleRightsText = "—";
			nextCollectionText = "—";
			storageTitleText = "STORED ITEMS · 0";
			emptyStorageText = "No vault data";
			return;
		}

		capacityText = vault.slotsUsed.ToString() + " / " + vault.capacity.ToString();
		capacitySubText = "slots used";
		dailyFeeText = vault.feePerSlotPerDay.ToString() + " /slot";
		feeSourceText = "debited from bank balance";
		capacityRoleText = "Solo";
		capacityRoleRightsText = "Full access";
		nextCollectionText = "Daily";

		UpdateCapacityProgressBar(vault.slotsUsed, vault.capacity);

		m_AllVaultItems.Clear();
		foreach (TXBVaultItem vi : vault.items)
		{
			if (vi) m_AllVaultItems.Insert(vi);
		}

		_vaultActivity.Clear();
		foreach (TXBVaultActivityEntry ae : vault.activity)
		{
			if (ae) _vaultActivity.Insert(ae);
		}

		if (m_ViewMode == "vault")
			ApplyVaultFilter();
	}

	// --- Storage grid modes ---

	protected void ApplyVaultFilter()
	{
		_visibleItems.Clear();
		string query = storageSearch;
		query.ToLower();
		foreach (TXBVaultItem vi : m_AllVaultItems)
		{
			if (!vi) continue;
			string lowerName = vi.displayName;
			lowerName.ToLower();
			if (query != "" && lowerName.IndexOf(query) < 0) continue;
			_visibleItems.Insert(vi);
		}
		storageTitleText = "STORED ITEMS · " + _visibleItems.Count().ToString();
		emptyStorageText = "No stored items";
	}

	protected void SwitchToInventoryMode()
	{
		m_ViewMode = "inventory";
		m_SelectedInventoryItemId = "";
		m_InventoryItems.Clear();
		m_InventoryEntities.Clear();
		BuildInventoryItemList();
		_visibleItems.Clear();
		foreach (TXBVaultItem ii : m_InventoryItems)
		{
			if (ii) _visibleItems.Insert(ii);
		}
		storageTitleText = "SELECT ITEM TO DEPOSIT";
		emptyStorageText = "No depositable items in inventory";
		actionsNoteText = "Click an item to preview it. Then click DEPOSIT FROM INVENTORY.";
		ClearDetailPanel();
		MarkForCheck();
	}

	protected void SwitchToVaultMode()
	{
		m_ViewMode = "vault";
		m_SelectedInventoryItemId = "";
		m_InventoryEntities.Clear();
		actionsNoteText = "Withdraw spawns into inventory. Deposit snapshots health/qty/attachments.";
		ApplyVaultFilter();
		ClearDetailPanel();
		MarkForCheck();
	}

	protected void BuildInventoryItemList()
	{
		PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
		if (!player) return;

		TXBSettings settings = GetTXBConfigService().GetClientSettings();

		array<EntityAI> items = new array<EntityAI>();
		player.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, items);

		int idx = 0;
		foreach (EntityAI item : items)
		{
			if (!item) continue;
			if (settings && settings.IsCurrency(item.GetType())) continue;

			TXBVaultItem pseudo = new TXBVaultItem();
			pseudo.id = "inv:" + idx.ToString();
			pseudo.className = item.GetType();
			pseudo.displayName = item.GetDisplayName();
			pseudo.healthLevel = ClientHealthToLevel(item.GetHealth01("", ""));
			pseudo.footprintW = 1;
			pseudo.footprintH = 1;
			pseudo.quantity = 1;
			pseudo.entityRef = item;
			m_InventoryItems.Insert(pseudo);
			m_InventoryEntities.Insert(item);
			idx = idx + 1;
		}
	}

	// --- Item row click ---

	void OnItemRowClicked(string itemId)
	{
		if (m_ViewMode == "inventory")
		{
			m_SelectedInventoryItemId = itemId;
			FindAndShowInventoryItem(itemId);
			actionsNoteText = "Click DEPOSIT FROM INVENTORY to store this item.";
			MarkForCheck();
			return;
		}
		m_SelectedItemId = itemId;
		PopulateDetailPanel(itemId);
		MarkForCheck();
	}

	// --- Deposit ---

	protected void DepositInventoryItem(string pseudoId)
	{
		if (m_Throttled) return;

		string className = "";
		foreach (TXBVaultItem ii : m_InventoryItems)
		{
			if (ii && ii.id == pseudoId)
			{
				className = ii.className;
				break;
			}
		}
		if (className == "") return;

		GetDayZularRPCManager().SendRPC(TXBConstants.RPC_NAMESPACE, "RequestVaultDeposit", new Param2<string, string>(className, m_CurrentOwnerKey), true, null);
		m_SelectedInventoryItemId = "";
		m_Throttled = true;
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(ClearThrottle, 1000, false);
		SwitchToVaultMode();
	}

	void OnDepositItem()
	{
		if (m_ViewMode == "vault")
		{
			SwitchToInventoryMode();
			return;
		}
		if (m_SelectedInventoryItemId != "")
		{
			DepositInventoryItem(m_SelectedInventoryItemId);
			return;
		}
		SwitchToVaultMode();
	}

	// --- Withdraw ---

	void OnWithdrawItem()
	{
		if (m_ViewMode != "vault" || m_SelectedItemId == "") return;
		if (m_Throttled) return;

		GetDayZularRPCManager().SendRPC(TXBConstants.RPC_NAMESPACE, "RequestVaultWithdraw", new Param2<string, string>(m_SelectedItemId, m_CurrentOwnerKey), true, null);
		m_SelectedItemId = "";
		m_Throttled = true;
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(ClearThrottle, 1000, false);
		ClearDetailPanel();
		MarkForCheck();
	}

	// --- Take all ---

	void OnTakeAll()
	{
		if (m_Throttled) return;
		GetDayZularRPCManager().SendRPC(TXBConstants.RPC_NAMESPACE, "RequestVaultTakeAll", new Param1<string>(m_CurrentOwnerKey), true, null);
		m_Throttled = true;
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(ClearThrottle, 2000, false);
		ClearDetailPanel();
		m_SelectedItemId = "";
		MarkForCheck();
	}

	// --- Search ---

	void OnStorageSearchChanged(string text)
	{
		if (m_ViewMode == "vault")
			ApplyVaultFilter();
		MarkForCheck();
	}

	// --- Detail panel ---

	protected void PopulateDetailPanel(string itemId)
	{
		TXBVaultItem found = null;
		foreach (TXBVaultItem vi : m_AllVaultItems)
		{
			if (vi && vi.id == itemId)
			{
				found = vi;
				break;
			}
		}
		if (!found)
		{
			ClearDetailPanel();
			return;
		}

		detailNameText = found.displayName;
		detailHealthText = HealthLevelLabel(found.healthLevel);
		detailClassText = found.className;
		detailFootprintText = found.footprintW.ToString() + "x" + found.footprintH.ToString();
		detailSinceText = FormatTimestamp(found.depositedUtc);
		detailDaysText = found.daysStored.ToString() + "d";
		detailAttachmentsText = TDZTernary(found.attachmentsSummary != "", found.attachmentsSummary, "—");
		detailCargoText = TDZTernary(found.cargoSummary != "", found.cargoSummary, "—");
		detailAccruedFeeText = found.accruedFee.ToString() + " roubles";
		detailDepositorText = "Deposited by " + found.depositorName + " · " + FormatTimestamp(found.depositedUtc);
		previewItem = null;
	}

	protected void FindAndShowInventoryItem(string pseudoId)
	{
		TXBVaultItem found = null;
		int entIdx = -1;
		int curIdx = 0;
		foreach (TXBVaultItem pseudo : m_InventoryItems)
		{
			if (pseudo && pseudo.id == pseudoId)
			{
				found = pseudo;
				entIdx = curIdx;
				break;
			}
			curIdx = curIdx + 1;
		}

		if (!found)
		{
			ClearDetailPanel();
			return;
		}

		detailNameText = found.displayName;
		detailHealthText = HealthLevelLabel(found.healthLevel);
		detailClassText = found.className;
		detailFootprintText = "—";
		detailSinceText = "—";
		detailDaysText = "—";
		detailAttachmentsText = "—";
		detailCargoText = "—";
		detailAccruedFeeText = "—";
		detailDepositorText = "Ready to deposit.";

		if (entIdx >= 0 && entIdx < m_InventoryEntities.Count())
			previewItem = m_InventoryEntities[entIdx];
		else
			previewItem = null;
	}

	protected void ClearDetailPanel()
	{
		detailNameText = "—";
		detailHealthText = "—";
		detailClassText = "—";
		detailFootprintText = "—";
		detailSinceText = "—";
		detailDaysText = "—";
		detailAttachmentsText = "—";
		detailCargoText = "—";
		detailAccruedFeeText = "—";
		detailDepositorText = "Select an item to see details.";
		previewItem = null;
	}

	// --- Capacity progress bar ---

	protected void UpdateCapacityProgressBar(int used, int capacity)
	{
		Widget root = GetRoot();
		if (!root) return;
		Widget fill = root.FindAnyWidget("capProgressFill");
		if (!fill) return;
		float ratio = 0;
		if (capacity > 0)
			ratio = Math.Clamp(used / (capacity * 1.0), 0, 1);
		fill.SetSize(ratio, 1);
	}

	// --- Throttle ---

	protected void ClearThrottle()
	{
		m_Throttled = false;
	}

	// --- Helpers ---

	protected string GetLocalPlayerUID()
	{
		PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
		if (!player) return "";
		PlayerIdentity identity = player.GetIdentity();
		if (!identity) return "";
		return identity.GetPlainId();
	}

	protected int ClientHealthToLevel(float h)
	{
		if (h >= 0.9) return 0;
		if (h >= 0.7) return 1;
		if (h >= 0.5) return 2;
		if (h >= 0.3) return 3;
		return 4;
	}

	protected string HealthLevelLabel(int level)
	{
		if (level == 0) return "PRISTINE";
		if (level == 1) return "WORN";
		if (level == 2) return "DAMAGED";
		if (level == 3) return "BADLY";
		return "RUINED";
	}

	protected string FormatTimestamp(int stamp)
	{
		if (stamp <= 0) return "—";
		int y = stamp / 10000000000;
		int rem = stamp - y * 10000000000;
		int mo = rem / 100000000;
		rem = rem - mo * 100000000;
		int d = rem / 1000000;
		rem = rem - d * 1000000;
		int h = rem / 10000;
		rem = rem - h * 10000;
		int mn = rem / 100;
		return mo.ToString() + "/" + d.ToString() + " " + h.ToString() + ":" + mn.ToString();
	}

	protected string TDZTernary(bool cond, string t, string f)
	{
		if (cond) return t;
		return f;
	}
}
