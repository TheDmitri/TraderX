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
	string itemSellPrice;
	// Nasdara tile: price with the currency symbol, and the "IN CART · n" badge.
	string price_display;
	string cart_badge;
	Widget inCartBadge;
	string itemStock;
	string quantityStr;
	float itemVolume;
	EntityAI preview;
	int tradeMode;
	int categoryType;
	bool selectable;
	// True when the card is unselectable ONLY because the main item is out of stock (buy mode).
	// Customize (variants/attachments) stays reachable then: a variant may have its own stock.
	bool m_OutOfStockOnly;
	bool isFavable;
	bool isFav;
	// Buy mode only: every category of the open trader listing this product requires a licence
	// the player lacks. The card is greyed (licenseDimPanel + licenseLockIcon) but stays
	// inspectable (hover tooltip, right-click customize/preview); selecting it for purchase is
	// refused with a toast. The server enforces the same rule (ValidateCategoryLicenses).
	bool m_LicenseLocked;

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
	// Persistent customize affordance on the card (replaces the hover-only tooltip hints).
	// All three are buttons so they can raise their own labelled hover tooltip; customizeBtn
	// also acts as the discoverable customize click target (gear icon).
	ButtonWidget variantBtn;
	ButtonWidget attachBtn;
	ButtonWidget customizeBtn;
	Widget licenseDimPanel;
	ImageWidget licenseLockIcon;
	ref SimpleBtnTooltipComponent simpleTooltipComp;

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
		TraderXLicenseService.Event_OnPlayerLicensesChanged.Insert(OnPlayerLicensesChanged);
		CheckoutCardComponent.Event_OnCartQuantityChanged.Insert(OnCartQuantityChanged);
		Event_OnDestroyAllTooltips.Insert(DestroyTooltips);

		if (_inputProduct)
			Setup(_inputProduct, ETraderXCategoryType.NONE, true, true, false);
	}

	override void OnDestroy()
	{
		TraderXSelectionService.Event_OnItemSelectionChanged.Remove(OnItemSelectionChanged);
		TraderXTradingService.Event_OnTraderXResponseReceived.Remove(OnTraderXResponseReceived);
		TraderXProduct.Event_OnCheckoutPricingChanged.Remove(OnCheckoutPricingChanged);
		TraderXLicenseService.Event_OnPlayerLicensesChanged.Remove(OnPlayerLicensesChanged);
		CheckoutCardComponent.Event_OnCartQuantityChanged.Remove(OnCartQuantityChanged);
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
		UpdatePriceDisplay();
		itemSellPrice = GetSellPriceSuffix();
		SetItemHealth();
		UpdateCustomizeAffordance();
		ApplyLicenseState();

		if (item.isSelected)
			Event_OnItemCardSelectCallBack.Invoke(this);

		MarkForCheck();
		OnShow(true);
	}

	void ApplyLicenseState()
	{
		m_LicenseLocked = false;
		if (tradeMode == ETraderXTradeMode.BUY && _inputProduct)
			m_LicenseLocked = TraderXLicenseService.GetInstance().IsProductLicenseLocked(_inputProduct);

		if (licenseDimPanel) licenseDimPanel.Show(m_LicenseLocked);
		if (licenseLockIcon) licenseLockIcon.Show(m_LicenseLocked);
	}

	// Licence bought (or licences synced late): un-grey live.
	void OnPlayerLicensesChanged()
	{
		if (!_inputProduct) return;
		ApplyLicenseState();
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
		m_OutOfStockOnly = false;

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
				m_OutOfStockOnly = true;
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
			preview = EntityAI.Cast(GetGame().GetObjectByNetworkId(_inputProduct.playerItem.networkIdLow, _inputProduct.playerItem.networkIdHigh));
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

	// Secondary sell price shown next to the buy price on the purchase tab only:
	// buy price is the prominent green value, this is a small dim "sell N" in red.
	// Empty on the sell tab and for unsellable items, so the red widget renders nothing.
	string GetSellPriceSuffix()
	{
		if (tradeMode != ETraderXTradeMode.BUY)
			return string.Empty;

		int sellPrice = TraderXPricingService.GetInstance().GetPricePreview(_inputProduct.GetProductId(), false, 1, TraderXItemState.PRISTINE);
		if (sellPrice < 0)
			return string.Empty;

		return Widget.TranslateString("#tpm_sells_for") + " " + TraderXCurrencyService.GetInstance().GetTraderCurrencySymbol() + TraderXQuantityManager.GetFormattedMoneyAmount(sellPrice);
	}

	int CalculateAttachmentsPrice()
	{
		int price = 0;
		if (!_inputProduct.selectedAttachments) return price;
		foreach (string productId : _inputProduct.selectedAttachments)
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
			UpdateCartBadge();
			if (unselectedIcon) unselectedIcon.Show(!_inputProduct.isSelected && selectable && !m_LicenseLocked);
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
		// Locked by a category licence: explain instead of silently adding it to the cart
		// (deselecting a card that was somehow selected stays allowed).
		if (m_LicenseLocked && !_inputProduct.isSelected)
		{
			TraderXLicenseService.GetInstance().NotifyIfProductLocked(_inputProduct);
			return;
		}
		if (KeyState(KeyCode.KC_LSHIFT) == 0)
			SetItemSelected(!_inputProduct.isSelected);
		Event_OnItemCardClickEventCallBack.Invoke(this, DZTernaryInt.If(KeyState(KeyCode.KC_LSHIFT) != 0, ETraderXClickEvents.SHIFT_LCLICK, ETraderXClickEvents.LCLICK));
	}

	void OnItemCardRightClick()
	{
		if (!selectable && !m_OutOfStockOnly) return;
		Event_OnItemCardClickEventCallBack.Invoke(this, DZTernaryInt.If(KeyState(KeyCode.KC_LSHIFT) != 0, ETraderXClickEvents.SHIFT_RCLICK, ETraderXClickEvents.RCLICK));
	}

	void OnItemCardDoubleClick()
	{
		if (!selectable) return;
		if (m_LicenseLocked) return;
		Event_OnItemCardClickEventCallBack.Invoke(this, ETraderXClickEvents.DOUBLE_CLICK);
	}

	void OnItemCardMouseEnter()
	{
		if (!selectable) return;
		if (!_inputProduct.isSelected)
		{
			if (itemCardOutline) itemCardOutline.SetAlpha(0.5);
			if (selectable && !m_LicenseLocked)
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
			if (selectable && !m_LicenseLocked)
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
			// Variants/attachments state now lives on the card as persistent icons, and the
			// click hints are gone — the tooltip is a plain description reveal on image hover.
			buyTooltipComp.Setup(GetItemDescription());
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
		DestroySimpleTooltip();
	}

	// Persistent, always-visible customize signalling on the card. The variant/attachment
	// icons tell the player at a glance what the item supports (no hover needed), and the
	// customize button is a discoverable click target for the right-click customize action.
	// Only shown for buyable top-level products (not sub-rows in the customize page).
	void UpdateCustomizeAffordance()
	{
		bool hasV = _inputProduct.HasVariants();
		bool hasA = _inputProduct.HasAttachments();
		bool customizable = (hasV || hasA) && tradeMode == ETraderXTradeMode.BUY && categoryType == ETraderXCategoryType.NONE;

		if (variantBtn) variantBtn.Show(customizable && hasV);
		if (attachBtn) attachBtn.Show(customizable && hasA);
		if (customizeBtn) customizeBtn.Show(customizable);
	}

	// The affordance icons open Customize on the tab that matches what they depict, so a
	// click does what its icon/label promises. All route through the RCLICK path (handled by
	// the page components); the target tab is handed to the page via s_PendingTab.
	void OpenCustomizeOnTab(string tabSelector)
	{
		if (!selectable && !m_OutOfStockOnly) return;
		CustomizePageComponent.s_PendingTab = tabSelector;
		Event_OnItemCardClickEventCallBack.Invoke(this, ETraderXClickEvents.RCLICK);
	}

	void OnCustomizeClick() { OpenCustomizeOnTab("tx-customize-presets"); }
	void OnVariantClick()   { OpenCustomizeOnTab("tx-customize-variants"); }
	void OnAttachClick()    { OpenCustomizeOnTab("tx-customize-attachments"); }

	// Labelled hover tooltips on the affordance icons so their meaning is explicit
	// (gear = Customize, palette = Variants, wrench = Attachments).
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

	void UpdatePriceDisplay()
	{
		price_display = TraderXCurrencyService.GetInstance().GetTraderCurrencySymbol() + TraderXQuantityManager.GetFormattedMoneyAmount(itemPrice.ToInt());
	}

	// "IN CART · 2" while the product is in the cart (its multiplier is the cart quantity).
	void UpdateCartBadge()
	{
		if (!_inputProduct) return;
		bool inCart = _inputProduct.isSelected && selectable;
		if (inCartBadge) inCartBadge.Show(inCart);
		cart_badge = AppUpper("#tpm_in_cart") + " · " + _inputProduct.GetMultiplier().ToString();
		MarkForCheck();
	}

	static string AppUpper(string key)
	{
		string s = Widget.TranslateString(key);
		s.ToUpper();
		return s;
	}

	void OnCartQuantityChanged(TraderXProduct product)
	{
		if (_inputProduct && product && _inputProduct.productId == product.productId)
			UpdateCartBadge();
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
		UpdateCartBadge();
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
				UpdatePriceDisplay();
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
