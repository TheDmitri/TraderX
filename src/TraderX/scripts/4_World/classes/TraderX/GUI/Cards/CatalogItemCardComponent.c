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
		MarkForCheck();
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

	void OnTraderXResponseReceived(int response, TraderXTransactionResultCollection transactionResultCollection = null)
	{
		if (response == ETraderXResponse.ALL_STOCK_RECEIVED || response == ETraderXResponse.TRANSACTIONS)
			UpdateStock();
	}
}
