// TraderXBanking - Vault Service
// Server-side: load/save TXBVaultData, deposit/withdraw item snapshots.
// Client-side: cache synced from server, fires Event_OnVaultUpdated for UI.

class TXBVaultService extends DayZularService
{
	protected ref TXBVaultData m_ClientVault;

	static ref ScriptInvoker Event_OnVaultUpdated = new ScriptInvoker();

	// --- Client-side cache ---

	TXBVaultData GetClientVault()
	{
		return m_ClientVault;
	}

	void SetClientVault(TXBVaultData data)
	{
		m_ClientVault = data;
		Event_OnVaultUpdated.Invoke();
	}

	// --- Server: load vault from disk (creates default if missing) ---

	TXBVaultData LoadVault(string ownerKey)
	{
		if (!GetGame().IsServer()) return null;

		string path = VaultPath(ownerKey);
		TXBVaultData data;

		if (FileExist(path))
		{
			data = new TXBVaultData();
			JsonFileLoader<TXBVaultData>.JsonLoadFile(path, data);
		}
		else
		{
			data = new TXBVaultData();
			data.ownerKey = ownerKey;
			int colonIdx = ownerKey.IndexOf(":");
			if (colonIdx >= 0)
			{
				data.ownerType = ownerKey.Substring(0, colonIdx);
				data.ownerId = ownerKey.Substring(colonIdx + 1, ownerKey.Length() - colonIdx - 1);
			}
			data.capacity = TXBConstants.VAULT_DEFAULT_CAPACITY;
			data.feePerSlotPerDay = TXBConstants.VAULT_DEFAULT_FEE_PER_DAY;
		}

		return data;
	}

	void SaveVault(TXBVaultData data)
	{
		if (!GetGame().IsServer()) return;
		if (!data) return;

		if (!FileExist(TXBConstants.VAULT_DIR))
			MakeDirectory(TXBConstants.VAULT_DIR);

		JsonFileLoader<TXBVaultData>.JsonSaveFile(VaultPath(data.ownerKey), data);
	}

	// --- Server: serialize vault and push to client ---

	void SendVaultToClient(PlayerBase player, string ownerKey)
	{
		if (!GetGame().IsServer()) return;
		if (!player) return;

		TXBVaultData vault = LoadVault(ownerKey);
		if (!vault) return;

		string json = "";
		JsonSerializer js = new JsonSerializer();
		js.WriteToString(vault, false, json);

		GetDayZularRPCManager().SendRPC(TXBConstants.RPC_NAMESPACE, "OnVaultReceived", new Param1<string>(json), false, player.GetIdentity());
	}

	// --- Server: deposit first matching className item from player inventory ---

	bool DepositItem(PlayerBase player, string className, string ownerKey)
	{
		if (!GetGame().IsServer()) return false;
		if (!player || className == "") return false;

		TXBVaultData vault = LoadVault(ownerKey);
		if (!vault) return false;
		if (vault.slotsUsed >= vault.capacity) return false;

		EntityAI item = FindItemInInventory(player, className);
		if (!item) return false;

		string depositorUid = "";
		string depositorName = "";
		PlayerIdentity identity = player.GetIdentity();
		if (identity)
		{
			depositorUid = identity.GetPlainId();
			depositorName = identity.GetName();
		}

		TXBVaultItem snapshot = SnapshotItem(item, depositorUid, depositorName);
		vault.items.Insert(snapshot);
		vault.slotsUsed = vault.slotsUsed + 1;

		TXBVaultActivityEntry act = new TXBVaultActivityEntry();
		act.id = snapshot.id + "_dep";
		act.timestampUtc = snapshot.depositedUtc;
		act.actorUid = depositorUid;
		act.actorName = depositorName;
		act.description = "Deposited " + snapshot.displayName;
		act.deltaText = "+1";
		act.activityType = 0;
		vault.activity.Insert(act);

		while (vault.activity.Count() > 50)
			vault.activity.RemoveOrdered(0);

		GetGame().ObjectDelete(item);
		SaveVault(vault);
		return true;
	}

	// --- Server: withdraw item by id from vault to player inventory ---

	bool WithdrawItem(PlayerBase player, string itemId, string ownerKey)
	{
		if (!GetGame().IsServer()) return false;
		if (!player || itemId == "") return false;

		TXBVaultData vault = LoadVault(ownerKey);
		if (!vault) return false;

		TXBVaultItem found = null;
		int foundIdx = -1;
		for (int i = 0; i < vault.items.Count(); i++)
		{
			if (vault.items[i] && vault.items[i].id == itemId)
			{
				found = vault.items[i];
				foundIdx = i;
				break;
			}
		}

		if (!found) return false;

		if (!RecreateItem(player, found)) return false;

		vault.items.RemoveOrdered(foundIdx);
		vault.slotsUsed = Math.Max(0, vault.slotsUsed - 1);

		string actorUid = "";
		string actorName = "";
		PlayerIdentity wIdentity = player.GetIdentity();
		if (wIdentity)
		{
			actorUid = wIdentity.GetPlainId();
			actorName = wIdentity.GetName();
		}

		TXBVaultActivityEntry wAct = new TXBVaultActivityEntry();
		wAct.id = itemId + "_with";
		wAct.timestampUtc = GetCurrentUtcStamp();
		wAct.actorUid = actorUid;
		wAct.actorName = actorName;
		wAct.description = "Withdrew " + found.displayName;
		wAct.deltaText = "-1";
		wAct.activityType = 1;
		vault.activity.Insert(wAct);

		while (vault.activity.Count() > 50)
			vault.activity.RemoveOrdered(0);

		SaveVault(vault);
		return true;
	}

	// --- Server: withdraw all items ---

	int TakeAll(PlayerBase player, string ownerKey)
	{
		if (!GetGame().IsServer()) return 0;
		if (!player) return 0;

		TXBVaultData vault = LoadVault(ownerKey);
		if (!vault || vault.items.Count() == 0) return 0;

		array<string> withdrawn = new array<string>();
		foreach (TXBVaultItem vi : vault.items)
		{
			if (!vi) continue;
			if (RecreateItem(player, vi))
				withdrawn.Insert(vi.id);
		}

		for (int i = vault.items.Count() - 1; i >= 0; i--)
		{
			if (vault.items[i] && withdrawn.Find(vault.items[i].id) >= 0)
				vault.items.RemoveOrdered(i);
		}

		vault.slotsUsed = vault.items.Count();

		if (withdrawn.Count() > 0)
		{
			string taUid = "";
			string taName = "";
			PlayerIdentity taIdentity = player.GetIdentity();
			if (taIdentity)
			{
				taUid = taIdentity.GetPlainId();
				taName = taIdentity.GetName();
			}
			TXBVaultActivityEntry taAct = new TXBVaultActivityEntry();
			taAct.id = "takeall_" + taUid + "_" + GetCurrentUtcStamp().ToString();
			taAct.timestampUtc = GetCurrentUtcStamp();
			taAct.actorUid = taUid;
			taAct.actorName = taName;
			taAct.description = "Withdrew all (" + withdrawn.Count().ToString() + " items)";
			taAct.deltaText = "-" + withdrawn.Count().ToString();
			taAct.activityType = 1;
			vault.activity.Insert(taAct);

			while (vault.activity.Count() > 50)
				vault.activity.RemoveOrdered(0);

			SaveVault(vault);
		}

		return withdrawn.Count();
	}

	// --- Snapshot: build display record + full recursive entity state ---

	TXBVaultItem SnapshotItem(EntityAI item, string depositorUid, string depositorName)
	{
		TXBVaultItem snap = new TXBVaultItem();
		snap.id = GenerateItemId(depositorUid, item.GetType());
		snap.className = item.GetType();
		snap.displayName = item.GetDisplayName();
		snap.footprintW = 1;
		snap.footprintH = 1;
		snap.depositorUid = depositorUid;
		snap.depositorName = depositorName;
		snap.depositedUtc = GetCurrentUtcStamp();

		snap.snapshotData = CaptureItemData(item);
		snap.healthLevel = HealthToLevel(snap.snapshotData.health);
		if (snap.snapshotData.quantity > 0)
			snap.quantity = Math.Round(snap.snapshotData.quantity);

		// Build display summaries for UI
		GameInventory inv = item.GetInventory();
		int attCount = inv.AttachmentCount();
		string attStr = "";
		for (int i = 0; i < attCount; i++)
		{
			EntityAI att = inv.GetAttachmentFromIndex(i);
			if (!att) continue;
			if (attStr != "") attStr = attStr + " · ";
			attStr = attStr + att.GetDisplayName();
		}
		snap.attachmentsSummary = attStr;

		CargoBase cargo = inv.GetCargo();
		if (cargo && cargo.GetItemCount() > 0)
			snap.cargoSummary = cargo.GetItemCount().ToString() + " item(s)";

		return snap;
	}

	// Recursively capture full item state into TXBVaultItemData tree.
	protected TXBVaultItemData CaptureItemData(EntityAI item)
	{
		TXBVaultItemData data = new TXBVaultItemData();
		data.className = item.GetType();

		// Inventory location (used when restoring children into parent slots)
		InventoryLocation loc = new InventoryLocation();
		item.GetInventory().GetCurrentInventoryLocation(loc);
		data.slot = loc.GetSlot();
		data.locType = loc.GetType();
		data.locIdx = loc.GetIdx();
		data.locRow = loc.GetRow();
		data.locCol = loc.GetCol();
		data.locFlip = loc.GetFlip();

		// Normalized health — works for all EntityAI without a cast
		data.health = item.GetHealth01("", "");

		// ItemBase-specific: quantity, liquid, temperature, wetness, energy
		ItemBase itemBs;
		if (Class.CastTo(itemBs, item))
		{
			data.quantity = itemBs.GetQuantity();
			data.temp = itemBs.GetTemperature();
			data.wetness = itemBs.GetWet();
			if (itemBs.IsLiquidContainer())
				data.liquidType = itemBs.GetLiquidType();
			ComponentEnergyManager em = itemBs.GetCompEM();
			if (em) data.energy = em.GetEnergy();
		}

		// Magazine ammo count
		Magazine itemMag;
		if (Class.CastTo(itemMag, item))
			data.magAmmoCount = itemMag.GetAmmoCount();

		// Recurse into cargo then attachments — store recursive result in local
		// before Insert() to avoid parser issues with inline ref-return calls
		GameInventory inv = item.GetInventory();
		int ci;
		int ai;
		CargoBase cargo = inv.GetCargo();
		if (cargo)
		{
			int cargoCount = cargo.GetItemCount();
			for (ci = 0; ci < cargoCount; ci++)
			{
				EntityAI cargoItem = cargo.GetItem(ci);
				if (cargoItem)
				{
					TXBVaultItemData cargoSnap = CaptureItemData(cargoItem);
					data.children.Insert(cargoSnap);
				}
			}
		}
		int attCount = inv.AttachmentCount();
		for (ai = 0; ai < attCount; ai++)
		{
			EntityAI att = inv.GetAttachmentFromIndex(ai);
			if (att)
			{
				TXBVaultItemData attSnap = CaptureItemData(att);
				data.children.Insert(attSnap);
			}
		}

		return data;
	}

	// --- Recreate: spawn item and restore full state ---

	protected bool RecreateItem(PlayerBase player, TXBVaultItem snap)
	{
		if (!snap || snap.className == "") return false;

		EntityAI spawned = EntityAI.Cast(GetGame().CreateObjectEx(snap.className, player.GetPosition(), ECE_PLACE_ON_SURFACE));
		if (!spawned) return false;

		if (snap.snapshotData)
		{
			// Apply root item state before moving to inventory
			ApplyItemData(spawned, snap.snapshotData);
			if (player.GetInventory().CanAddEntityToInventory(spawned))
				player.GetInventory().TakeEntityToInventory(InventoryMode.SERVER, FindInventoryLocationType.CARGO, spawned);
			// Restore attachments + cargo after item is in inventory
			if (snap.snapshotData.children.Count() > 0)
				RestoreItemChildren(spawned, snap.snapshotData.children);
		}
		else
		{
			// Legacy path: vault entry predates snapshotData
			float healthVal = LevelToHealth(snap.healthLevel);
			spawned.SetHealth01("", "", healthVal);
			if (spawned.HasQuantity() && snap.quantity > 0)
				spawned.SetQuantity(snap.quantity);
			if (player.GetInventory().CanAddEntityToInventory(spawned))
				player.GetInventory().TakeEntityToInventory(InventoryMode.SERVER, FindInventoryLocationType.CARGO, spawned);
		}

		return true;
	}

	// Apply state fields from TXBVaultItemData onto an already-spawned entity.
	protected void ApplyItemData(EntityAI item, TXBVaultItemData data)
	{
		item.SetHealth01("", "", data.health);

		ItemBase itemBs;
		if (Class.CastTo(itemBs, item))
		{
			if (data.quantity > 0) itemBs.SetQuantity(data.quantity);
			if (data.liquidType > 0) itemBs.SetLiquidType(data.liquidType);
			itemBs.SetTemperature(data.temp);
			itemBs.SetWet(data.wetness);
			ComponentEnergyManager em = itemBs.GetCompEM();
			if (em) em.SetEnergy(data.energy);
		}

		Magazine itemMag;
		if (Class.CastTo(itemMag, item))
			itemMag.ServerSetAmmoCount(data.magAmmoCount);
	}

	// Recursively recreate children (attachments + cargo) into a parent entity.
	protected void RestoreItemChildren(EntityAI parent, array<ref TXBVaultItemData> children)
	{
		GameInventory inv = parent.GetInventory();
		if (!inv) return;

		for (int i = 0; i < children.Count(); i++)
		{
			TXBVaultItemData childData = children[i];
			if (!childData || childData.className == "") continue;

			EntityAI child = null;
			if (childData.locType == InventoryLocationType.ATTACHMENT)
				child = inv.CreateAttachmentEx(childData.className, childData.slot);
			else
				child = inv.CreateEntityInCargoEx(childData.className, childData.locIdx, childData.locRow, childData.locCol, childData.locFlip);

			if (!child) continue;

			ApplyItemData(child, childData);

			if (childData.children.Count() > 0)
				RestoreItemChildren(child, childData.children);
		}
	}

	// --- Item search helper ---

	protected EntityAI FindItemInInventory(PlayerBase player, string className)
	{
		array<EntityAI> items = new array<EntityAI>();
		player.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, items);
		foreach (EntityAI item : items)
		{
			if (item && item.GetType() == className)
				return item;
		}
		return null;
	}

	// --- Health level helpers (for display only) ---

	protected int HealthToLevel(float h)
	{
		if (h >= 0.9) return 0;
		if (h >= 0.7) return 1;
		if (h >= 0.5) return 2;
		if (h >= 0.3) return 3;
		return 4;
	}

	protected float LevelToHealth(int level)
	{
		if (level == 0) return 1.0;
		if (level == 1) return 0.8;
		if (level == 2) return 0.6;
		if (level == 3) return 0.4;
		return 0.1;
	}

	// --- Path + ID helpers ---

	protected string VaultPath(string ownerKey)
	{
		string sanitized = ownerKey;
		sanitized.Replace(":", "_");
		return TXBConstants.VAULT_DIR + "Vault_" + sanitized + ".json";
	}

	protected string GenerateItemId(string uid, string className)
	{
		string uidPart = uid.Substring(0, Math.Min(8, uid.Length()));
		string clsPart = className.Substring(0, Math.Min(8, className.Length()));
		return uidPart + "_" + clsPart + "_" + GetCurrentUtcStamp().ToString();
	}

	int GetCurrentUtcStamp()
	{
		int y;
		int mo;
		int d;
		int h;
		int mn;
		int s;
		GetYearMonthDayUTC(y, mo, d);
		GetHourMinuteSecondUTC(h, mn, s);
		return y * 10000000000 + mo * 100000000 + d * 1000000 + h * 10000 + mn * 100 + s;
	}
}

TXBVaultService GetTXBVaultService()
{
	return TXBVaultService.Cast(DZlrServiceRegistry.GetInstance().GetService(TXBVaultService));
}
