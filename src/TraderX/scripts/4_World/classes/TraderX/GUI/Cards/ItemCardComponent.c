// Shared base for the item-card variants (small/large/tooltip + catalog uses its
// own). Not registered/mounted directly — only its subclasses are. Keeps all the
// binding/selection/tooltip logic; subclasses just set their template.
class ItemCardComponent extends DZlrComponentBase
{
	ref TraderXProduct _inputProduct;
	// Last product the card was built for — lets OnChanges detect when the ForEach binding
	// re-binds this card to a different product (e.g. after a sort) and refresh accordingly.
	protected TraderXProduct m_BoundProduct;

	string itemName;
	string itemPrice;
	string itemStock;
	string quantityStr;
	float itemVolume;
	EntityAI preview;
	int tradeMode;
	int categoryType;
	bool selectable;
	bool isFavable;
	bool isFav;

	Widget itemCardOutline;
	Widget itemCardContent;
	Widget stockText;
	Widget stockGrid;
	Widget volumeBar;
	Widget favBtn;
	ImageWidget selectedIcon;
	ImageWidget unselectedIcon;
	ImageWidget favIcon;
	ImageWidget unfavIcon;
	ImageWidget healthImg;
	ButtonWidget itemCardMainBtn;
	ButtonWidget volumeBtn;

	ref NavBtnTooltipComponent navBtnTooltipComp;
	ref SellTooltipComponent sellTooltipComp;
	ref BuyTooltipComponent buyTooltipComp;
	ref CustomizeTooltipComponent customizeTooltipComp;

	static ref ScriptInvoker Event_OnItemCardClickEventCallBack = new ScriptInvoker();
	static ref ScriptInvoker Event_OnItemCardSelectCallBack = new ScriptInvoker();
	static ref ScriptInvoker Event_OnDestroyAllTooltips = new ScriptInvoker();

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "tx-item-card";
		m_Metadata.templatePath = "TraderX/datasets/gui/TraderXPrefab/ItemCard/ItemCardLargeView.layout";
	}

	override void OnInit()
	{
		AutoLoadWidgets();
		TraderXSelectionService.Event_OnItemSelectionChanged.Insert(OnItemSelectionChanged);
		TraderXTradingService.Event_OnTraderXResponseReceived.Insert(OnTraderXResponseReceived);
		TraderXProduct.Event_OnCheckoutPricingChanged.Insert(OnCheckoutPricingChanged);
		Event_OnDestroyAllTooltips.Insert(DestroyTooltips);

		if (_inputProduct)
			Setup(_inputProduct, ETraderXCategoryType.NONE, true, true, false);
	}

	override void OnDestroy()
	{
		TraderXSelectionService.Event_OnItemSelectionChanged.Remove(OnItemSelectionChanged);
		TraderXTradingService.Event_OnTraderXResponseReceived.Remove(OnTraderXResponseReceived);
		TraderXProduct.Event_OnCheckoutPricingChanged.Remove(OnCheckoutPricingChanged);
		Event_OnDestroyAllTooltips.Remove(DestroyTooltips);
		DestroyTooltips();
	}

	static void DestroyAllTooltips()
	{
		Event_OnDestroyAllTooltips.Invoke();
	}

	// Re-bound by the ForEach binding (count unchanged → it reassigns each card's item and
	// calls OnChanges). Rebuild the card when the product actually changed — this is what
	// makes the price sort visibly reorder the cards.
	override void OnChanges()
	{
		if (_inputProduct && _inputProduct != m_BoundProduct)
			Setup(_inputProduct, categoryType, selectable, isFavable, false);
	}

	void Setup(TraderXProduct item, int catType, bool sel, bool favable, bool fav)
	{
		_inputProduct = item;
		m_BoundProduct = item;
		tradeMode = TraderXTradingService.GetInstance().GetTradeMode();
		categoryType = catType;
		selectable = sel;
		isFavable = favable;

		if (isFavable)
		{
			isFav = TraderXFavoritesService.GetInstance().IsFavorite(item);
			UpdateFav();
		}

		ShowName();
		UpdateStock();
		bool showBar = GetItemVolumePerCent(itemVolume);
		if (volumeBar) volumeBar.Show(showBar);

		CreatePreview();
		itemPrice = GetPriceFromItem();
		SetItemHealth();

		if (item.isSelected)
			Event_OnItemCardSelectCallBack.Invoke(this);

		MarkForCheck();
		OnShow(true);
	}

	void ShowName()
	{
		itemName = _inputProduct.GetDisplayName();
	}

	bool IsStockEmpty()
	{
		return TraderXProductStockRepository.GetStockValue(_inputProduct.productId) == 0;
	}

	void UpdateStock()
	{
		if (_inputProduct.IsStockUnlimited())
		{
			// Unlimited stock has no count to show — hide the whole stock grid (icon + text).
			if (stockGrid) stockGrid.Show(false);
			if (stockText) stockText.Show(false);
			return;
		}

		if (stockGrid) stockGrid.Show(true);

		// NOTE: don't tint itemCardContent here. In the grid it's a TXSurface-styled
		// widget (the compiler forces it white so the style shows); in the list it's
		// an unstyled transparent holder over the TXListRow row — painting it white
		// turned the whole row white. Warning state is shown via stockText (red).
		selectable = true;

		if (tradeMode == ETraderXTradeMode.SELL)
		{
			if (TraderXProductStockRepository.IsStockReached(_inputProduct.productId, _inputProduct.maxStock))
			{
				if (stockText) TextWidget.Cast(stockText).SetColor(TraderXTheme.STOCK_LOW);
				selectable = false;
			}
		}

		if (tradeMode == ETraderXTradeMode.BUY)
		{
			if (TraderXProductStockRepository.GetStockValue(_inputProduct.productId) == 0)
			{
				if (stockText) TextWidget.Cast(stockText).SetColor(TraderXTheme.STOCK_LOW);
				selectable = false;
			}
		}

		itemStock = TraderXProductStockRepository.GetStockDisplay(_inputProduct.productId, _inputProduct.maxStock);
		MarkForCheck();
	}

	void SetItemHealth()
	{
		int a, r, g, b;
		if (tradeMode == ETraderXTradeMode.SELL)
		{
			ItemBase itemB = ItemBase.Cast(GetGame().GetObjectByNetworkId(_inputProduct.playerItem.networkIdLow, _inputProduct.playerItem.networkIdHigh));
			if (!itemB) return;
			float health = itemB.GetHealthLevel();
			TraderXCoreUtils.GetHealthFromLevel(health, a, r, g, b);
			if (healthImg) healthImg.SetColor(ARGB(a, r, g, b));
		}
	}

	void CreatePreview()
	{
		if (tradeMode == ETraderXTradeMode.SELL)
		{
			preview = GetGame().GetObjectByNetworkId(_inputProduct.playerItem.networkIdLow, _inputProduct.playerItem.networkIdHigh);
			if (!preview)
				preview = EntityAI.Cast(GetGame().CreateObjectEx(_inputProduct.className, vector.Zero, ECE_LOCAL|ECE_NOLIFETIME));
		}
		else
		{
			preview = EntityAI.Cast(GetGame().CreateObjectEx(_inputProduct.className, vector.Zero, ECE_LOCAL|ECE_NOLIFETIME));
			LookForDefaultPreset();
			CreateAttachments();
		}
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

	EntityAI GetPreview()
	{
		return preview;
	}

	TraderXProduct GetItem()
	{
		return _inputProduct;
	}

	int GetPrice()
	{
		return itemPrice.ToInt();
	}

	string GetPriceFromItem()
	{
		if (tradeMode == ETraderXTradeMode.SELL)
		{
			if (_inputProduct.GetPlayerItem())
			{
				int dynamicPrice = TraderXPricingService.GetInstance().GetPricePreview(_inputProduct.GetProductId(), false, 1, _inputProduct.GetPlayerItem().healthLevel);
				return dynamicPrice.ToString();
			}
			else
			{
				int catalogSellPrice = TraderXPricingService.GetInstance().GetPricePreview(_inputProduct.GetProductId(), false, 1, TraderXItemState.PRISTINE);
				return catalogSellPrice.ToString();
			}
		}

		if (tradeMode == ETraderXTradeMode.BUY)
		{
			int basePrice = TraderXPricingService.GetInstance().GetPricePreview(_inputProduct.GetProductId(), true, 1, TraderXItemState.PRISTINE);
			if (_inputProduct.defaultPreset)
			{
				basePrice = TraderXPresetsService.GetInstance().CalculateTotalPricePreset(_inputProduct.defaultPreset);
			}
			else
			{
				basePrice += CalculateAttachmentsPrice();
			}
			return basePrice.ToString();
		}

		return string.Empty;
	}

	int CalculateAttachmentsPrice()
	{
		int price = 0;
		if (!_inputProduct.selectedAttachments) return price;
		foreach (UUID productId : _inputProduct.selectedAttachments)
		{
			TraderXProduct attachment = TraderXProductRepository.GetItemById(productId);
			if (!attachment) continue;
			price += TraderXPricingService.GetInstance().GetPricePreview(attachment.GetProductId(), true, 1, TraderXItemState.PRISTINE);
		}
		return price;
	}

	bool GetItemVolumePerCent(out float volume)
	{
		ItemBase itemB;
		if (tradeMode == ETraderXTradeMode.SELL)
		{
			itemB = ItemBase.Cast(GetGame().GetObjectByNetworkId(_inputProduct.playerItem.networkIdLow, _inputProduct.playerItem.networkIdHigh));
			if (!itemB) return false;
			if (!itemB.HasQuantity()) return false;
			volume = TraderXQuantityManager.GetItemVolumePerCent(itemB);
			return true;
		}

		if (tradeMode == ETraderXTradeMode.BUY)
		{
			if (!preview) return false;
			itemB = ItemBase.Cast(preview);
			if (!itemB) return false;
			if (!itemB.HasQuantity()) return false;
			int tradeQuantity = TraderXTradeQuantity.GetItemBuyQuantityClient(preview, _inputProduct.tradeQuantity);
			int maxQuantity = TraderXQuantityManager.GetMaxItemQuantityClient(itemB);
			if (maxQuantity <= 0) return false;
			quantityStr = tradeQuantity.ToString() + "/" + maxQuantity;
			volume = Math.Round((tradeQuantity * 100) / maxQuantity);
			return true;
		}

		return false;
	}

	string GetItemDescription()
	{
		if (!preview) return string.Empty;
		InventoryItem iItem = InventoryItem.Cast(preview);
		if (iItem) return TraderXCoreUtils.TrimUnt(iItem.GetTooltip());
		return string.Empty;
	}

	void OnShow(bool show)
	{
		if (show)
		{
			if (favIcon) favIcon.Show(isFav && isFavable);
			if (unfavIcon) unfavIcon.Show(!isFav && isFavable);
			if (selectedIcon) selectedIcon.Show(_inputProduct.isSelected && selectable);
			if (itemCardOutline) itemCardOutline.SetAlpha(DZTernaryFloat.If(_inputProduct.isSelected, 1.0, 0.0));
			if (unselectedIcon) unselectedIcon.Show(!_inputProduct.isSelected && selectable);
			if (healthImg) healthImg.Show(tradeMode == ETraderXTradeMode.SELL);
		}
		else
		{
			DestroyTooltips();
		}
	}

	// Left/right click are wired separately in the layout (ButtonBinding OnClick /
	// OnRightClick). DayZular's HandleOnClick rejects the RIGHT button, so a
	// right-click never reaches OnItemCardLeftClick — the two are independent.
	void OnItemCardLeftClick()
	{
		if (!selectable) return;
		if (KeyState(KeyCode.KC_LSHIFT) == 0)
			SetItemSelected(!_inputProduct.isSelected);
		Event_OnItemCardClickEventCallBack.Invoke(this, DZTernaryInt.If(KeyState(KeyCode.KC_LSHIFT) != 0, ETraderXClickEvents.SHIFT_LCLICK, ETraderXClickEvents.LCLICK));
	}

	void OnItemCardRightClick()
	{
		if (!selectable) return;
		Event_OnItemCardClickEventCallBack.Invoke(this, DZTernaryInt.If(KeyState(KeyCode.KC_LSHIFT) != 0, ETraderXClickEvents.SHIFT_RCLICK, ETraderXClickEvents.RCLICK));
	}

	void OnItemCardDoubleClick()
	{
		if (!selectable) return;
		Event_OnItemCardClickEventCallBack.Invoke(this, ETraderXClickEvents.DOUBLE_CLICK);
	}

	void OnItemCardMouseEnter()
	{
		if (!selectable) return;
		if (!_inputProduct.isSelected)
		{
			if (itemCardOutline) itemCardOutline.SetAlpha(0.5);
			if (selectable)
			{
				if (unselectedIcon) unselectedIcon.Show(false);
				if (selectedIcon)
				{
					selectedIcon.Show(true);
					selectedIcon.SetAlpha(0.5);
				}
			}
		}
		ShowTooltip();
	}

	void OnItemCardMouseLeave()
	{
		if (!_inputProduct.isSelected)
		{
			if (itemCardOutline) itemCardOutline.SetAlpha(0.0);
			if (selectable)
			{
				if (unselectedIcon) unselectedIcon.Show(true);
				if (selectedIcon)
				{
					selectedIcon.Show(false);
					selectedIcon.SetAlpha(0);
				}
			}
		}
		DestroyTooltips();
	}

	void ShowTooltip()
	{
		if (!preview) return;

		if (tradeMode == ETraderXTradeMode.BUY && categoryType == ETraderXCategoryType.NONE)
		{
			buyTooltipComp = new BuyTooltipComponent();
			buyTooltipComp.InitComponent(null);
			buyTooltipComp.Setup(GetItemDescription(), _inputProduct.HasVariants(), _inputProduct.HasAttachments());
			return;
		}

		if (tradeMode == ETraderXTradeMode.BUY && categoryType != ETraderXCategoryType.NONE)
		{
			customizeTooltipComp = new CustomizeTooltipComponent();
			customizeTooltipComp.InitComponent(null);
			customizeTooltipComp.Setup(GetItemDescription());
			return;
		}

		if (tradeMode == ETraderXTradeMode.SELL)
		{
			sellTooltipComp = new SellTooltipComponent();
			sellTooltipComp.InitComponent(null);
			sellTooltipComp.Setup(GetItemDescription(), TraderXInventoryManager.GetEntityHierarchy(preview));
			return;
		}
	}

	void DestroyTooltips()
	{
		if (buyTooltipComp) { buyTooltipComp.Destroy(); buyTooltipComp = null; }
		if (sellTooltipComp) { sellTooltipComp.Destroy(); sellTooltipComp = null; }
		if (customizeTooltipComp) { customizeTooltipComp.Destroy(); customizeTooltipComp = null; }
	}

	void OnItemFavClick()
	{
		SwitchFav();
		if (isFav)
		{
			TraderXFavoritesService.GetInstance().AddFavorite(_inputProduct);
			return;
		}
		TraderXFavoritesService.GetInstance().RemoveFavorite(_inputProduct);
	}

	void SetItemSelected(bool selected)
	{
		TraderXSelectionService selectionService = TraderXSelectionService.GetInstance();
		if (selected)
			selectionService.SelectItem(_inputProduct);
		else
			selectionService.DeselectItem(_inputProduct);
	}

	void SwitchFav()
	{
		isFav = !isFav;
		UpdateFav();
	}

	void UpdateFav()
	{
		if (unfavIcon) unfavIcon.Show(!isFav);
		if (favIcon) favIcon.Show(isFav);
	}

	void UpdateVisuals()
	{
		if (selectedIcon) selectedIcon.Show(_inputProduct.isSelected);
		if (unselectedIcon) unselectedIcon.Show(!_inputProduct.isSelected);
		if (itemCardOutline) itemCardOutline.SetAlpha(DZTernaryFloat.If(_inputProduct.isSelected, 1.0, 0.0));
		GetRoot().Update();
	}

	void OnItemSelectionChanged(TraderXProduct updatedItem)
	{
		if (!_inputProduct || !updatedItem) return;
		if (_inputProduct.productId == updatedItem.productId)
			UpdateVisuals();
	}

	void OnCheckoutPricingChanged()
	{
		if (tradeMode == ETraderXTradeMode.SELL)
		{
			string newPrice = GetPriceFromItem();
			if (newPrice != itemPrice)
			{
				itemPrice = newPrice;
				MarkForCheck();
			}
		}
	}

	void OnTraderXResponseReceived(int response, TraderXTransactionResultCollection transactionResultCollection = null)
	{
		if (response == ETraderXResponse.ALL_STOCK_RECEIVED || response == ETraderXResponse.TRANSACTIONS)
			UpdateStock();
	}
}

class ItemCardSmallComponent extends ItemCardComponent
{
	private static ref ComponentAutoRegister s_RegisterSmall = new ComponentAutoRegister("tx-item-card-small", ItemCardSmallComponent);

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "tx-item-card-small";
		m_Metadata.templatePath = "TraderX/datasets/gui/TraderXPrefab/ItemCard/ItemCardSmallView.layout";
	}
}

class ItemCardLargeComponent extends ItemCardComponent
{
	private static ref ComponentAutoRegister s_RegisterLarge = new ComponentAutoRegister("tx-item-card-large", ItemCardLargeComponent);

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "tx-item-card-large";
		m_Metadata.templatePath = "TraderX/datasets/gui/TraderXPrefab/ItemCard/ItemCardLargeView.layout";
	}
}

class ItemCardTooltipComponent extends ItemCardComponent
{
	private static ref ComponentAutoRegister s_RegisterTooltip = new ComponentAutoRegister("tx-item-card-tooltip", ItemCardTooltipComponent);

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "tx-item-card-tooltip";
		m_Metadata.templatePath = "TraderX/datasets/gui/TraderXPrefab/ItemCard/ItemCardMediumViewTooltip.layout";
	}
}
