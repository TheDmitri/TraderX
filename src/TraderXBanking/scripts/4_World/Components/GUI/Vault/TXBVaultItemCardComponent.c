// TraderXBanking - Vault Item Card Component
// ForEach card template for the 4-column vault storage grid.
// entityRef is populated for inventory pseudo-items (deposit flow) - the
// ItemPreviewWidget renders the live entity. Vault-stored items have null entityRef
// so the preview shows nothing (item is server-side only).

class TXBVaultItemCardComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("txb-vault-item-card", TXBVaultItemCardComponent);

	static ref ScriptInvoker Event_OnItemClicked = new ScriptInvoker();

	// Input (ForEach item property)
	ref TXBVaultItem _inputItem;

	// Display bindings
	string displayName;
	string qtyText;
	string footprintText;
	string healthText;
	EntityAI entityRef;

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "txb-vault-item-card";
		m_Metadata.templatePath = "TraderXBanking/datasets/layouts/VaultItemCard.layout";
	}

	override void OnInit()
	{
		if (!_inputItem)
		{
			displayName = "-";
			qtyText = "";
			footprintText = "";
			healthText = "";
			entityRef = null;
			MarkForCheck();
			return;
		}

		displayName = _inputItem.displayName;
		qtyText = FormatQty(_inputItem.quantity);
		footprintText = _inputItem.footprintW.ToString() + "x" + _inputItem.footprintH.ToString();
		healthText = HealthLevelLabel(_inputItem.healthLevel);
		entityRef = SpawnPreviewEntity(_inputItem);

		MarkForCheck();
	}

	override void OnDestroy()
	{
		// Delete the local preview clone when the card is torn down (foreach rebuild / close).
		// Clear the ItemPreviewWidget FIRST so it never renders a freed entity (native CTD).
		if (entityRef)
		{
			Widget root = GetRoot();
			if (root)
			{
				ItemPreviewWidget pw = ItemPreviewWidget.Cast(root.FindAnyWidget("cardPreview"));
				if (pw) pw.SetItem(null);
			}
			GetGame().ObjectDelete(entityRef);
			entityRef = null;
		}
	}

	// ItemPreviewWidget renders a spawned, out-of-hierarchy clone - NOT a live in-inventory item
	// (those render blank). Spawn a local, no-lifetime preview by class (works for stored + inventory).
	protected EntityAI SpawnPreviewEntity(TXBVaultItem item)
	{
		if (!item || item.className == "")
			return null;
		GetTXBLog().LogInfo("[TXB] card SpawnPreview: " + item.className);
		EntityAI e = EntityAI.Cast(GetGame().CreateObjectEx(item.className, vector.Zero, ECE_LOCAL | ECE_NOLIFETIME));
		GetTXBLog().LogInfo("[TXB] card SpawnPreview OK: " + item.className);
		return e;
	}

	void OnCardClicked()
	{
		if (!_inputItem) return;
		Event_OnItemClicked.Invoke(_inputItem.id);
	}

	protected string FormatQty(int q)
	{
		if (q <= 1) return "";
		return "x" + q.ToString();
	}

	protected string HealthLevelLabel(int level)
	{
		if (level == 0) return "PRISTINE";
		if (level == 1) return "WORN";
		if (level == 2) return "DAMAGED";
		if (level == 3) return "BADLY DAMAGED";
		return "RUINED";
	}
}
