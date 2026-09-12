class CheckoutCardComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("tx-checkout-card", CheckoutCardComponent);

	static ref ScriptInvoker Event_OnPriceChanged = new ScriptInvoker();

	ref TraderXProduct _inputProduct;

	string product_name_text;
	string product_quantity_text;
	string product_price_text;

	// Preset picker (BUY, non-customize, product with presets). The options are shown in a
	// collapsible list that expands the card downward (a floating dropdown gets clipped by
	// the checkout scroll). ForEach source + the current selection label for the toggle.
	ref array<ref TXCheckoutPresetOption> _presetOptions = new array<ref TXCheckoutPresetOption>();
	string preset_toggle_label;
	private bool m_PresetsExpanded;

	private int price;
	private int quantity;
	private int tradeMode;

	Widget btnPlus;
	Widget btnMinus;
	Widget removeBtn;
	Widget presetToggleBtn;      // "Presets" row toggle (shown only when options exist)
	Widget presetOptionsGrid;    // collapsible ForEach list, hidden until expanded
	ImageWidget presetChevron;

	private string m_NoPresetLabel;

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "tx-checkout-card";
		m_Metadata.templatePath = "TraderX/datasets/gui/TraderXPrefab/Checkout/CheckoutCardView.layout";
	}

	override void OnInit()
	{
		AutoLoadWidgets();
		if (_inputProduct)
			Setup(_inputProduct);
	}

	void Setup(TraderXProduct item, int qty = 1)
	{
		_inputProduct = item;
		quantity = qty;
		tradeMode = TraderXTradingService.GetInstance().GetTradeMode();
		price = GetBasePrice();

		TraderXProduct.Event_OnMultiplierChanged.Insert(OnProductMultiplierChanged);
		TraderXProduct.Event_OnCheckoutPricingChanged.Insert(OnCheckoutPricingChanged);

		// Only append " [preset]" when there's a non-empty preset name — avoids the
		// stray "[]" shown on sell-page checkout cards (player items have no preset).
		string presetSuffix = "";
		if (item.defaultPreset && item.defaultPreset.presetName != string.Empty)
			presetSuffix = " [" + item.defaultPreset.presetName + "]";

		if (item.GetPlayerItem())
		{
			product_name_text = item.GetDisplayName() + presetSuffix;
		}
		else
		{
			if (TraderXUINavigationService.GetInstance().GetNavigationId() == ENavigationIds.CUSTOMIZE)
			{
				product_name_text = item.GetDisplayName();
			}
			else
			{
				product_name_text = item.GetDisplayName() + presetSuffix;
			}
		}

		if (tradeMode == ETraderXTradeMode.SELL)
		{
			if (btnPlus) btnPlus.Show(false);
			if (btnMinus) btnMinus.Show(false);
		}

		SetupPresetPicker();

		RefreshQuantityAndPrice();
		MarkForCheck();
		OnSellQuantityModeChanged(TraderXTradingService.GetInstance().IsMaxQuantity());
	}

	// Build the collapsible preset picker for a normal product buy. Hidden for sell,
	// customize, player items, and products with no presets. Same-base presets only (a
	// preset whose productId differs would swap the base weapon — out of scope here).
	void SetupPresetPicker()
	{
		m_PresetsExpanded = false;
		if (presetToggleBtn) presetToggleBtn.Show(false);
		if (presetOptionsGrid) presetOptionsGrid.Show(false);

		if (tradeMode != ETraderXTradeMode.BUY) return;
		if (TraderXUINavigationService.GetInstance().GetNavigationId() == ENavigationIds.CUSTOMIZE) return;
		if (_inputProduct.GetPlayerItem()) return;

		_presetOptions.Clear();
		m_NoPresetLabel = Widget.TranslateString("#tpm_no_preset");

		TraderXPresets serverPresets = TraderXPresetsService.GetInstance().GetServerPresets(_inputProduct.productId);
		if (serverPresets && serverPresets.presets)
			AddPresetOptions(serverPresets.presets);

		AddPresetOptions(TraderXPresetsService.GetInstance().GetPresets(_inputProduct.productId));

		// No presets for this product -> keep the row simple, no toggle.
		if (_presetOptions.Count() == 0) return;

		// Leading "No preset" entry so the player can revert to a bare item.
		TXCheckoutPresetOption none = new TXCheckoutPresetOption();
		none.owner = this;
		none.preset = null;
		none.isNone = true;
		none.label = m_NoPresetLabel;
		_presetOptions.InsertAt(none, 0);

		// The select bar now carries the preset, so drop the redundant " [preset]" from the
		// item name.
		product_name_text = _inputProduct.GetDisplayName();

		UpdateToggleLabel();
		if (presetToggleBtn) presetToggleBtn.Show(true);
	}

	// Insert same-base presets as options, de-duplicating by name.
	void AddPresetOptions(array<ref TraderXPreset> presets)
	{
		if (!presets) return;
		foreach (TraderXPreset preset : presets)
		{
			if (!preset) continue;
			if (preset.productId != _inputProduct.productId) continue;   // same-base only
			if (preset.presetName == string.Empty) continue;
			if (HasOption(preset.presetName)) continue;

			TXCheckoutPresetOption opt = new TXCheckoutPresetOption();
			opt.owner = this;
			opt.preset = preset;
			opt.isNone = false;
			opt.label = preset.presetName;
			_presetOptions.Insert(opt);
		}
	}

	bool HasOption(string label)
	{
		foreach (TXCheckoutPresetOption o : _presetOptions)
		{
			if (o && o.label == label)
				return true;
		}
		return false;
	}

	// Toggle button in the main row shows the current selection.
	void UpdateToggleLabel()
	{
		if (_inputProduct.defaultPreset && _inputProduct.defaultPreset.presetName != string.Empty)
			preset_toggle_label = _inputProduct.defaultPreset.presetName;
		else
			preset_toggle_label = m_NoPresetLabel;
	}

	// Expand/collapse the option list below the row. The card root is size-to-content, so
	// showing the ForEach grid grows the card and the checkout list reflows below it.
	void OnPresetToggle()
	{
		m_PresetsExpanded = !m_PresetsExpanded;
		if (presetOptionsGrid) presetOptionsGrid.Show(m_PresetsExpanded);
		MarkForCheck();
	}

	// Apply the chosen preset to this line, reprice + rename, then collapse the list.
	void ApplyPresetOption(TXCheckoutPresetOption option)
	{
		if (!option) return;

		TraderXPreset chosen = option.preset;
		if (option.isNone)
			chosen = null;

		_inputProduct.defaultPreset = chosen;

		// Keep the spawned item's attachments in sync with the chosen preset (bare when none).
		if (chosen)
			_inputProduct.selectedAttachments = chosen.attachments;
		else
			_inputProduct.selectedAttachments = new array<string>();

		// Name stays clean; the select bar reflects the chosen preset.
		product_name_text = _inputProduct.GetDisplayName();

		UpdateToggleLabel();

		// Collapse after selection.
		m_PresetsExpanded = false;
		if (presetOptionsGrid) presetOptionsGrid.Show(false);

		RefreshQuantityAndPrice();
		MarkForCheck();
	}

	int GetTradeMode()
	{
		return tradeMode;
	}

	// Single source of truth for the BUY price at a given quantity. Customize prices the
	// bare product; otherwise a preset uses progressive preset pricing (which itself falls
	// back to the flat total at qty 1), else plain product pricing.
	int ComputeBuyPrice(int qty)
	{
		if (TraderXUINavigationService.GetInstance().GetNavigationId() == ENavigationIds.CUSTOMIZE)
			return TraderXPricingService.GetInstance().GetPricePreview(_inputProduct.GetProductId(), true, qty, TraderXItemState.PRISTINE);
		if (_inputProduct.defaultPreset)
			return TraderXPresetsService.GetInstance().CalculateProgressivePresetPrice(_inputProduct.defaultPreset, qty);
		return TraderXPricingService.GetInstance().GetPricePreview(_inputProduct.GetProductId(), true, qty, TraderXItemState.PRISTINE);
	}

	int GetBasePrice()
	{
		if (tradeMode == ETraderXTradeMode.BUY)
			return ComputeBuyPrice(1);

		if (tradeMode == ETraderXTradeMode.SELL)
			return TraderXPricingService.GetInstance().GetPricePreview(_inputProduct.GetProductId(), false, 1, _inputProduct.GetPlayerItem().healthLevel);

		return 0;
	}

	void SetQuantity(int qty)
	{
		quantity = qty;
		product_quantity_text = DZTernaryString.If(tradeMode == ETraderXTradeMode.BUY, "+", DZTernaryString.If(tradeMode == ETraderXTradeMode.SELL, "-", "")) + qty.ToString();
	}

	void SetPrice(int newPrice)
	{
		price = newPrice;
		if (tradeMode == ETraderXTradeMode.SELL)
		{
			if (price < 0) price = 0;
			if (price == 0)
			{
				product_price_text = "0";
			}
			else
			{
				product_price_text = "-" + TraderXQuantityManager.GetFormattedMoneyAmount(price);
			}
		}
		else
		{
			product_price_text = TraderXQuantityManager.GetFormattedMoneyAmount(price);
		}
	}

	int GetPrice()
	{
		return price;
	}

	TraderXProduct GetItem()
	{
		return _inputProduct;
	}

	void RefreshQuantityAndPrice()
	{
		SetQuantity(_inputProduct.GetMultiplier());

		int totalPrice = 0;
		if (tradeMode == ETraderXTradeMode.BUY)
		{
			totalPrice = ComputeBuyPrice(_inputProduct.GetMultiplier());
		}
		else if (tradeMode == ETraderXTradeMode.SELL)
		{
			TraderXPriceCalculation priceCalc;
			string itemKey = _inputProduct.GetPlayerItem().GetPlayerItemId();
			if (_inputProduct.defaultPreset)
			{
				priceCalc = TraderXCheckoutPricingService.GetInstance().CalculatePresetSellPriceWithCheckoutForItem(_inputProduct.defaultPreset, itemKey, _inputProduct.GetMultiplier(), _inputProduct.GetPlayerItem().healthLevel);
			}
			else
			{
				priceCalc = TraderXCheckoutPricingService.GetInstance().CalculateSellPriceWithCheckoutForItem(TraderXProductRepository.GetItemById(_inputProduct.GetProductId()), itemKey, _inputProduct.GetMultiplier(), _inputProduct.GetPlayerItem().healthLevel);
			}
			totalPrice = priceCalc.GetCalculatedPrice();
		}

		SetPrice(totalPrice);
		MarkForCheck();
		Event_OnPriceChanged.Invoke();
	}

	void OnProductMultiplierChanged(TraderXProduct product)
	{
		RefreshQuantityAndPrice();
	}

	void OnCheckoutPricingChanged()
	{
		if (tradeMode == ETraderXTradeMode.SELL)
			RefreshQuantityAndPrice();
	}

	void ResetPriceQuantityToNominal()
	{
		SetQuantity(1);
		SetPrice(GetBasePrice());
		_inputProduct.SetMultiplier(1);
	}

	void OnSellQuantityModeChanged(bool isMaxQuantity)
	{
		ResetPriceQuantityToNominal();
		if (!isMaxQuantity) return;

		int sellMode = TraderXTradeQuantity.GetSellQuantityMode(_inputProduct.tradeQuantity);
		if (sellMode != TraderXTradeQuantity.SELL_STATIC && sellMode != TraderXTradeQuantity.SELL_COEFFICIENT)
			return;

		ItemBase itemB = ItemBase.Cast(GetGame().GetObjectByNetworkId(_inputProduct.playerItem.networkIdLow, _inputProduct.playerItem.networkIdHigh));
		if (!itemB) return;

		int itemQuantity = TraderXQuantityManager.GetItemAmount(itemB);
		int tradeQty = TraderXTradeQuantity.GetItemSellQuantity(itemB.GetType(), _inputProduct.tradeQuantity);
		if (tradeQty == 0) return;

		int newTradeQty = tradeQty;
		while ((itemQuantity - tradeQty) > 0)
		{
			itemQuantity -= tradeQty;
			newTradeQty += tradeQty;
		}

		_inputProduct.SetMultiplier(newTradeQty / tradeQty);
		RefreshQuantityAndPrice();
	}

	void OnPlusClick()
	{
		if (tradeMode == ETraderXTradeMode.BUY)
		{
			_inputProduct.SetMultiplier(_inputProduct.GetMultiplier() + 1);
			RefreshQuantityAndPrice();
		}
	}

	void OnMinusClick()
	{
		if (tradeMode == ETraderXTradeMode.BUY)
		{
			if (_inputProduct.GetMultiplier() == 1) return;
			_inputProduct.SetMultiplier(_inputProduct.GetMultiplier() - 1);
			RefreshQuantityAndPrice();
		}
	}

	void OnRemoveClick()
	{
		CheckoutComponent checkout = CheckoutComponent.GetInstance();
		if (!checkout || !_inputProduct)
			return;

		// In customize, the main weapon line's trash clears all attachments but
		// keeps the weapon itself. Attachment rows remove only themselves.
		if (TraderXUINavigationService.GetInstance().GetNavigationId() == ENavigationIds.CUSTOMIZE && CustomizeStateService.GetInstance().IsMainItem(_inputProduct))
		{
			checkout.RemoveAllAttachmentsKeepMain();
			return;
		}

		checkout.DeselectAndRemoveItem(_inputProduct);
	}
}
