class CatalogItemCardComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("tx-catalog-item-card", CatalogItemCardComponent);

	static ref ScriptInvoker Event_OnCatalogItemCardClickCallBack = new ScriptInvoker();

	ref TraderXProduct _inputProduct;
	protected TraderXProduct m_BoundProduct;

	string itemName;
	string itemPrice;
	string itemStock;
	int categoryType;
	EntityAI preview;

	Widget itemCardContent;
	Widget stockText;
	ImageWidget healthImg;
	ButtonWidget itemCardMainBtn;

	// Persistent customize affordance (palette = variants, wrench = attachments, gear =
	// customize). All buttons so each can raise its own labelled hover tooltip; the gear
	// also opens the customize page via the RCLICK event.
	ButtonWidget variantBtn;
	ButtonWidget attachBtn;
	ButtonWidget customizeBtn;
	ref SimpleBtnTooltipComponent simpleTooltipComp;

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "tx-catalog-item-card";
		m_Metadata.templatePath = "TraderX/datasets/gui/TraderXPrefab/ItemCard/CatalogItemCardView.layout";
	}

	override void OnInit()
	{
		AutoLoadWidgets();
		TraderXTradingService.Event_OnTraderXResponseReceived.Insert(OnTraderXResponseReceived);

		if (_inputProduct)
			Setup(_inputProduct, ETraderXCategoryType.NONE);
	}

	override void OnDestroy()
	{
		TraderXTradingService.Event_OnTraderXResponseReceived.Remove(OnTraderXResponseReceived);
		DestroySimpleTooltip();
	}

	// Rebound by the ForEach binding after a sort — refresh when the product changed so the
	// list visibly reorders.
	override void OnChanges()
	{
		if (_inputProduct && _inputProduct != m_BoundProduct)
			Setup(_inputProduct, categoryType);
	}

	void Setup(TraderXProduct item, int catType)
	{
		_inputProduct = item;
		m_BoundProduct = item;
		categoryType = catType;
		ShowName();
		UpdateStock();
		CreatePreview();
		itemPrice = GetSellPrice();
		UpdateCustomizeAffordance();
		MarkForCheck();
	}

	void UpdateCustomizeAffordance()
	{
		bool hasV = _inputProduct.HasVariants();
		bool hasA = _inputProduct.HasAttachments();
		bool customizable = hasV || hasA;

		if (variantBtn) variantBtn.Show(customizable && hasV);
		if (attachBtn) attachBtn.Show(customizable && hasA);
		if (customizeBtn) customizeBtn.Show(customizable);
	}

	void ShowName()
	{
		itemName = _inputProduct.GetDisplayName();
	}

	void UpdateStock()
	{
		if (_inputProduct.IsStockUnlimited())
		{
			if (stockText) stockText.Show(false);
		}
		else
		{
			itemStock = TraderXProductStockRepository.GetStockDisplay(_inputProduct.productId, _inputProduct.maxStock);
			if (stockText) stockText.Show(true);
		}
		MarkForCheck();
	}

	void CreatePreview()
	{
		preview = EntityAI.Cast(GetGame().CreateObjectEx(_inputProduct.className, vector.Zero, ECE_LOCAL|ECE_NOLIFETIME));
		LookForDefaultPreset();
		CreateAttachments();
		MarkForCheck();
	}

	void LookForDefaultPreset()
	{
		TraderXPreset defaultPreset = TraderXPresetsService.GetInstance().GetDefaultPreset(_inputProduct.productId);
		if (!defaultPreset)
			defaultPreset = TraderXPresetsService.GetInstance().GetServerDefaultPreset(_inputProduct.productId);
		if (!defaultPreset) return;
		_inputProduct.defaultPreset = defaultPreset;
		_inputProduct.selectedAttachments = defaultPreset.attachments;
	}

	void CreateAttachments()
	{
		if (!_inputProduct || !_inputProduct.selectedAttachments) return;
		foreach (string productId : _inputProduct.selectedAttachments)
		{
			TraderXProduct attachment = TraderXProductRepository.GetItemById(productId);
			if (!attachment) continue;
			EntityAI entAttach = EntityAI.Cast(GetGame().CreateObjectEx(attachment.className, vector.Zero, ECE_LOCAL|ECE_NOLIFETIME));
			if (!entAttach) continue;
			TraderXAttachmentHandler.TryAttachItem(preview, entAttach);
		}
	}

	string GetSellPrice()
	{
		int sellPrice = TraderXPricingService.GetInstance().GetPricePreview(_inputProduct.GetProductId(), false, 1, TraderXItemState.PRISTINE);
		return sellPrice.ToString();
	}

	int GetPrice()
	{
		return TraderXPricingService.GetInstance().GetPricePreview(_inputProduct.GetProductId(), false, 1, TraderXItemState.PRISTINE);
	}

	TraderXProduct GetItem()
	{
		return _inputProduct;
	}

	EntityAI GetPreview()
	{
		return preview;
	}

	string GetItemDescription()
	{
		if (!preview) return string.Empty;
		InventoryItem iItem = InventoryItem.Cast(preview);
		if (iItem) return TraderXCoreUtils.TrimUnt(iItem.GetTooltip());
		return string.Empty;
	}

	void OnCatalogItemClick(int button)
	{
		if (button == MouseState.LEFT)
			Event_OnCatalogItemCardClickCallBack.Invoke(this, ETraderXClickEvents.LCLICK);
		if (button == MouseState.RIGHT)
			Event_OnCatalogItemCardClickCallBack.Invoke(this, ETraderXClickEvents.RCLICK);
	}

	void OnCatalogItemDoubleClick()
	{
		Event_OnCatalogItemCardClickCallBack.Invoke(this, ETraderXClickEvents.DOUBLE_CLICK);
	}

	// The affordance icons open Customize on the tab that matches what they depict, so a
	// click does what its icon/label promises. All route through the RCLICK path (handled by
	// CatalogPageComponent); the target tab is handed to the page via s_PendingTab.
	void OpenCustomizeOnTab(string tabSelector)
	{
		CustomizePageComponent.s_PendingTab = tabSelector;
		Event_OnCatalogItemCardClickCallBack.Invoke(this, ETraderXClickEvents.RCLICK);
	}

	void OnCustomizeClick() { OpenCustomizeOnTab("tx-customize-presets"); }
	void OnVariantClick()   { OpenCustomizeOnTab("tx-customize-variants"); }
	void OnAttachClick()    { OpenCustomizeOnTab("tx-customize-attachments"); }

	// Labelled hover tooltips on the affordance icons (gear = Customize, palette = Variants,
	// wrench = Attachments) so their meaning is explicit.
	void ShowSimpleTooltip(string label)
	{
		DestroySimpleTooltip();
		simpleTooltipComp = new SimpleBtnTooltipComponent();
		simpleTooltipComp.InitComponent(null);
		simpleTooltipComp.Setup(label);
	}

	void DestroySimpleTooltip()
	{
		if (simpleTooltipComp) { simpleTooltipComp.Destroy(); simpleTooltipComp = null; }
	}

	void OnCustomizeEnter() { ShowSimpleTooltip("#tpm_customize"); }
	void OnVariantEnter()   { ShowSimpleTooltip("#tpm_variants"); }
	void OnAttachEnter()    { ShowSimpleTooltip("#tpm_attachments"); }
	void OnAffordanceLeave() { DestroySimpleTooltip(); }

	void OnTraderXResponseReceived(int response, TraderXTransactionResultCollection transactionResultCollection = null)
	{
		if (response == ETraderXResponse.ALL_STOCK_RECEIVED || response == ETraderXResponse.TRANSACTIONS)
			UpdateStock();
	}
}
