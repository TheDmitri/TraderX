class CheckoutComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("tx-checkout", CheckoutComponent);

	static ref ScriptInvoker Event_OnTotalPriceChange = new ScriptInvoker();

	int totalPrice;
	string total_amount;
	string bank_amount;
	string money_amount;
	string checkout_label;
	string checkout_btn_text;
	// Nasdara cart: "CART", summary lines, empty state with the beans shortcut.
	string checkout_title;
	string summary_label_1;
	string summary_value_1;
	string summary_label_2;
	string summary_value_2;
	string empty_cart_text;
	string add_beans_text;
	Widget addBeansHost;
	// customize mode: no cash summary, Save as preset above the buy button
	Widget checkoutFooter;
	Widget summaryLabel1;
	Widget summaryValue1;
	Widget summaryLabel2;
	Widget summaryValue2;
	Widget LabelTotal;
	Widget totalAmount;
	Widget savePresetHost;
	Widget purchaseBtnHost;
	Widget CheckoutScroll;
	protected int m_Cash;
	protected int m_Bank;
	protected bool m_BankUsable;
	// The trader's baked beans product, when it sells one (empty-cart shortcut).
	protected TraderXProduct m_BeansProduct;

	ref array<ref TraderXProduct> _inputCheckoutItems = new array<ref TraderXProduct>();

	Widget emptyCartPanel;

	private static ref CheckoutComponent m_Instance;

	// The last buy/sell collection sent to the server, kept so a "not enough space" rejection
	// can be re-sent with ground-drop confirmed once the player accepts the prompt.
	private ref TraderXTransactionCollection m_PendingTransactions;
	private int m_PendingNpcId;

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "tx-checkout";
		m_Metadata.templatePath = "TraderX/datasets/gui/TraderXPrefab/Checkout/CheckoutView.layout";
	}

	static CheckoutComponent GetInstance()
	{
		return m_Instance;
	}

	// The products currently in the cart (used e.g. by CustomizeStateService to
	// collect selected attachments).
	array<ref TraderXProduct> GetCheckoutItems()
	{
		return _inputCheckoutItems;
	}

	override void OnInit()
	{
		m_Instance = this;
		AutoLoadWidgets();
		TraderXSelectionService.Event_OnItemSelectionChanged.Insert(OnItemSelectionChanged);
		SellPageComponent.Event_OnSellQuantityModeChanged.Insert(OnSellQuantityModeChanged);
		TraderXTradingService.Event_OnTraderXResponseReceived.Insert(OnTraderXResponseReceived);
		CheckoutCardComponent.Event_OnPriceChanged.Insert(OnPriceChanged);
		UpdateTradeModeText();
		// Rebuild the cart from the persistent selection so it survives page swaps
		// (e.g. returning from the customize page).
		UpdateCheckoutView();
	}

	bool IsSellMode()
	{
		return TraderXTradingService.GetInstance() && TraderXTradingService.GetInstance().GetTradeMode() == ETraderXTradeMode.SELL;
	}

	static string Upper(string key)
	{
		string s = Widget.TranslateString(key);
		s.ToUpper();
		return s;
	}

	void UpdateTradeModeText()
	{
		if (IsSellMode())
			checkout_label = "#tpm_income";
		else
			checkout_label = "#tpm_to_pay";
		checkout_title = Upper("#tpm_cart");
		if (IsCustomize()) checkout_title = Upper("#tpm_configuration");
		ApplyModeLayout();
		FindBeansProduct();
		UpdateSummary();
		MarkForCheck();
	}

	bool IsCustomize()
	{
		return TraderXUINavigationService.GetInstance().GetNavigationId() == ENavigationIds.CUSTOMIZE;
	}

	// Footer: purchase = cash summary, TOTAL, BUY (226 px); customize = TOTAL,
	// Save as preset, BUY (237 px). The footer is bottom-anchored, so it grows upward.
	// Layout pixels -> script units. Script SetPos/SetSize pixel values are not in the
	// layout's units on scaled displays, so every value below is the 1080p design value
	// times k, measured once from the footer's own layout height (226).
	protected float m_UiK;

	void ApplyModeLayout()
	{
		bool customize = IsCustomize();
		if (m_UiK <= 0 && checkoutFooter)
		{
			float fw, fh;
			checkoutFooter.GetSize(fw, fh);
			m_UiK = fh / 226.0;
		}
		float k = m_UiK;
		if (k <= 0) k = 1;
		if (summaryLabel1) summaryLabel1.Show(!customize);
		if (summaryValue1) summaryValue1.Show(!customize);
		if (summaryLabel2) summaryLabel2.Show(!customize);
		if (summaryValue2) summaryValue2.Show(!customize);
		if (savePresetHost) savePresetHost.Show(customize);
		if (checkoutFooter) checkoutFooter.SetSize(1, k * DZTernaryFloat.If(customize, 237, 226));
		if (LabelTotal) LabelTotal.SetPos(k * 24, k * DZTernaryFloat.If(customize, 24, 88));
		if (totalAmount) totalAmount.SetPos(k * 24, k * DZTernaryFloat.If(customize, 22, 86));
		if (purchaseBtnHost) purchaseBtnHost.SetPos(k * 24, k * DZTernaryFloat.If(customize, 146, 139));
		if (CheckoutScroll) CheckoutScroll.SetSize(1, k * DZTernaryFloat.If(customize, 635, 646));
	}

	void OnSavePresetClick()
	{
		TraderXInventoryManager.PlayMenuSound(ETraderXSounds.SELECT);
		if (CustomizePageComponent.GetInstance())
			CustomizePageComponent.GetInstance().OpenSavePreset();
	}

	// Empty cart (DayZ Beans Launcher placement 3): the "Add Baked Beans" shortcut shows
	// only when this trader sells the baked beans can, and adds that real product.
	void FindBeansProduct()
	{
		m_BeansProduct = null;
		if (!IsSellMode() && !IsCustomize() && TraderXTradingService.GetInstance())
		{
			array<ref TraderXCategory> categories = TraderXTradingService.GetInstance().GetTraderCategories();
			foreach (TraderXCategory category : categories)
			{
				if (!category || m_BeansProduct) continue;
				// keep the returned array alive while iterating (1.30 frees temporaries)
				array<ref TraderXProduct> beansCandidates = category.GetProducts();
				foreach (TraderXProduct product : beansCandidates)
				{
					if (!product || !product.CanBeBought()) continue;
					string cls = product.className;
					cls.ToLower();
					if (cls == "bakedbeanscan")
					{
						m_BeansProduct = product;
						break;
					}
				}
			}
		}
		string symbol = TraderXCurrencyService.GetInstance().GetTraderCurrencySymbol();
		if (m_BeansProduct)
		{
			int beansPrice = TraderXPricingService.GetInstance().GetPricePreview(m_BeansProduct.GetProductId(), true, 1, TraderXItemState.PRISTINE);
			add_beans_text = Widget.TranslateString("#tpm_add_baked_beans") + " · " + symbol + TraderXQuantityManager.GetFormattedMoneyAmount(beansPrice);
			empty_cart_text = Widget.TranslateString("#tpm_cart_empty_hint") + " " + Widget.TranslateString("#tpm_cart_empty_beans");
		}
		else
		{
			empty_cart_text = Widget.TranslateString("#tpm_cart_empty_hint");
		}
		if (addBeansHost) addBeansHost.Show(m_BeansProduct != null);
	}

	void OnAddBeansClick()
	{
		if (!m_BeansProduct || m_BeansProduct.isSelected) return;
		TraderXInventoryManager.PlayMenuSound(ETraderXSounds.SELECT);
		TraderXSelectionService.GetInstance().SelectItem(m_BeansProduct);
	}

	// Summary lines + "BUY 3 ITEMS": purchases pay cash first, then the bank.
	void UpdateSummary()
	{
		string symbol = TraderXCurrencyService.GetInstance().GetTraderCurrencySymbol();
		int count = 0;
		foreach (TraderXProduct item : _inputCheckoutItems)
		{
			if (item) count += Math.Max(1, item.GetMultiplier());
		}
		string itemsWord = " " + count.ToString() + " " + Upper("#tpm_items");
		if (count == 1) itemsWord = " 1 " + Upper("#tpm_item");
		if (count == 0) itemsWord = "";
		if (IsCustomize())
		{
			checkout_btn_text = Upper("#tpm_buy_configuration");
		}
		else if (IsSellMode())
		{
			checkout_btn_text = Upper("#tpm_sell") + itemsWord;
			summary_label_1 = Widget.TranslateString("#tpm_you_receive");
			summary_value_1 = symbol + TraderXQuantityManager.GetFormattedMoneyAmount(totalPrice);
			summary_label_2 = Widget.TranslateString("#tpm_cash_after");
			summary_value_2 = symbol + TraderXQuantityManager.GetFormattedMoneyAmount(m_Cash + totalPrice);
		}
		else
		{
			checkout_btn_text = Upper("#tpm_buy") + itemsWord;
			int fromCash = Math.Min(totalPrice, m_Cash);
			int fromBank = totalPrice - fromCash;
			summary_label_1 = Widget.TranslateString("#tpm_paid_from_cash");
			summary_value_1 = symbol + TraderXQuantityManager.GetFormattedMoneyAmount(fromCash);
			if (fromBank > 0 && m_BankUsable)
			{
				summary_label_2 = Widget.TranslateString("#tpm_paid_from_bank");
				summary_value_2 = symbol + TraderXQuantityManager.GetFormattedMoneyAmount(fromBank);
			}
			else
			{
				summary_label_2 = Widget.TranslateString("#tpm_cash_after");
				summary_value_2 = symbol + TraderXQuantityManager.GetFormattedMoneyAmount(m_Cash - fromCash);
			}
		}
		total_amount = symbol + TraderXQuantityManager.GetFormattedMoneyAmount(totalPrice);
	}

	override void OnDestroy()
	{
		TraderXSelectionService.Event_OnItemSelectionChanged.Remove(OnItemSelectionChanged);
		SellPageComponent.Event_OnSellQuantityModeChanged.Remove(OnSellQuantityModeChanged);
		TraderXTradingService.Event_OnTraderXResponseReceived.Remove(OnTraderXResponseReceived);
		CheckoutCardComponent.Event_OnPriceChanged.Remove(OnPriceChanged);
		// Drop any pending no-space modal subscriptions so the static invokers don't retain
		// this destroyed component.
		CleanupNoSpaceModal();
		m_Instance = null;
	}

	void AddCheckoutItem(TraderXProduct item)
	{
		if (!HasItem(item))
		{
			if (item.GetPlayerItem())
			{
				string uniqueKey = item.GetPlayerItem().GetPlayerItemId();
				TraderXCheckoutPricingService.GetInstance().AddItemToCheckout(item.GetProductId(), item.GetMultiplier(), uniqueKey);
			}
			_inputCheckoutItems.Insert(item);
			UpdateEmptyState();
			MarkForCheck();
		}
	}

	bool HasItem(TraderXProduct item)
	{
		foreach (TraderXProduct existing : _inputCheckoutItems)
		{
			if (existing == item) return true;
			if (item.GetPlayerItem() && existing.GetPlayerItem())
			{
				if (item.GetPlayerItem().GetPlayerItemId() == existing.GetPlayerItem().GetPlayerItemId())
					return true;
			}
			else if (!item.GetPlayerItem() && !existing.GetPlayerItem())
			{
				if (item.productId == existing.productId)
					return true;
			}
		}
		return false;
	}

	void RemoveCheckoutItem(TraderXProduct item)
	{
		for (int i = _inputCheckoutItems.Count() - 1; i >= 0; i--)
		{
			if (item.GetPlayerItem())
			{
				if (item.GetPlayerItem().GetPlayerItemId() == _inputCheckoutItems[i].GetPlayerItem().GetPlayerItemId())
				{
					TraderXCheckoutPricingService.GetInstance().RemoveItemFromCheckout(item.GetProductId(), item.GetMultiplier());
					_inputCheckoutItems.Remove(i);
					UpdateEmptyState();
					MarkForCheck();
					return;
				}
			}
			else
			{
				if (item.productId == _inputCheckoutItems[i].productId)
				{
					_inputCheckoutItems.Remove(i);
					UpdateEmptyState();
					MarkForCheck();
					return;
				}
			}
		}
	}

	void DeselectAndRemoveItem(TraderXProduct item)
	{
		if (!item) return;

		// The main weapon line is protected in customize — only a variant swap
		// (which goes through TraderXSelectionService directly) may replace it.
		if (TraderXUINavigationService.GetInstance().GetNavigationId() == ENavigationIds.CUSTOMIZE && CustomizeStateService.GetInstance().IsMainItem(item))
			return;

		item.isSelected = false;
		TraderXSelectionService.GetInstance().DeselectItem(item);
	}

	// Remove every checkout item except the customize main line (weapon/variant).
	// Snapshot first — DeselectAndRemoveItem mutates _inputCheckoutItems.
	void RemoveAllAttachmentsKeepMain()
	{
		array<ref TraderXProduct> toRemove = new array<ref TraderXProduct>();
		foreach (TraderXProduct it : _inputCheckoutItems)
		{
			if (it && !CustomizeStateService.GetInstance().IsMainItem(it))
				toRemove.Insert(it);
		}

		foreach (TraderXProduct r : toRemove)
		{
			DeselectAndRemoveItem(r);
		}
	}

	void OnItemSelectionChanged(TraderXProduct item)
	{
		if (item.isSelected)
			AddCheckoutItem(item);
		else
			RemoveCheckoutItem(item);
		UpdateTotalPrice();
		UpdatePlayerMoney();
	}

	void UpdatePlayerMoney()
	{
		PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
		if (!player) return;
		// Menu teardown: the trader is already released, nothing to show.
		if (TraderXTradingService.GetInstance().GetNpcId() < 0) return;
		int playerMoney = TraderXTradingService.GetInstance().GetPlayerMoneyAmount(player);
		int bankBalance = 0;
		bool bankUsable = false;
		if (TraderXProviderRegistry.GetInstance().HasBanking())
		{
			ITraderXPaymentProvider bankProv = TraderXProviderRegistry.GetInstance().GetBankingProvider();
			if (bankProv && bankProv.IsAvailable(player))
			{
				bankUsable = true;
				bankBalance = bankProv.GetClientBalance();
			}
		}
		bank_amount = TraderXQuantityManager.GetFormattedMoneyAmount(bankBalance);
		// Show cash and bank distinctly (purchases pay cash first, then the bank).
		string cashText = TraderXQuantityManager.GetFormattedMoneyAmount(playerMoney);
		if (bankUsable)
			money_amount = cashText + " + " + bank_amount + " bank";
		else
			money_amount = cashText;
		m_Cash = playerMoney;
		m_Bank = bankBalance;
		m_BankUsable = bankUsable;
		UpdateSummary();
		MarkForCheck();
	}

	void UpdateCheckoutView()
	{
		// AddCheckoutItem re-registers player items into the checkout pricing service, so
		// the rebuild must start from a clean slate — otherwise every refresh accumulates
		// duplicate stock-change entries and the sell-with-checkout price drifts.
		_inputCheckoutItems.Clear();
		TraderXCheckoutPricingService.GetInstance().ClearCheckout();
		array<ref TraderXProduct> selectedItems = TraderXSelectionService.GetInstance().GetSelectedItems();
		foreach (TraderXProduct item : selectedItems)
			AddCheckoutItem(item);
		UpdateTotalPrice();
		UpdatePlayerMoney();
	}

	void OnSellQuantityModeChanged(bool isMaxQuantity)
	{
		UpdateTotalPrice();
	}

	void OnPriceChanged()
	{
		UpdateTotalPrice();
	}

	int GetTotalPrice()
	{
		totalPrice = 0;
		// The ForEach binding inserts each tx-checkout-card into m_ChildComponents;
		// sum the price each card computed for its product.
		foreach (DZlrComponentBase child : m_ChildComponents)
		{
			CheckoutCardComponent card = CheckoutCardComponent.Cast(child);
			if (card)
				totalPrice += card.GetPrice();
		}
		return totalPrice;
	}

	void UpdateTotalPrice()
	{
		totalPrice = GetTotalPrice();
		UpdateSummary();
		UpdateEmptyState();
		MarkForCheck();
	}

	// "Add items to cart" placeholder only shows while the cart is empty.
	void UpdateEmptyState()
	{
		if (emptyCartPanel)
			emptyCartPanel.Show(_inputCheckoutItems.Count() == 0);
	}

	void OnTraderXResponseReceived(int responseReceived, TraderXTransactionResultCollection transactionResultCollection = null)
	{
		if (responseReceived != ETraderXResponse.TRANSACTIONS || !transactionResultCollection) return;

		array<string> itemsToDeselect = new array<string>();
		for (int i = 0; i < transactionResultCollection.GetTransactionResults().Count(); i++)
		{
			TraderXTransactionResult transactionResult = transactionResultCollection.GetTransactionResults()[i];
			if (transactionResult.IsSuccess())
				itemsToDeselect.Insert(transactionResult.GetProductId());
		}

		if (itemsToDeselect.Count() > 0)
		{
			if (TraderXUINavigationService.GetInstance().GetNavigationId() != ENavigationIds.CUSTOMIZE)
				TraderXSelectionService.GetInstance().DeselectItems(itemsToDeselect);
			TraderXInventoryManager.PlayMenuSound(ETraderXSounds.COINS);
		}

		UpdateCheckoutView();

		// Lines rejected only for lack of inventory space were NOT bought or charged. They stay
		// in the cart (successful lines above were removed); offer a ground-drop confirm for them.
		if (transactionResultCollection.HasNoSpace())
			PromptGroundDropForNoSpace(transactionResultCollection);
	}

	void OnClearCheckoutClick()
	{
		TraderXInventoryManager.PlayMenuSound(ETraderXSounds.SELECT);
		if (_inputCheckoutItems.Count() == 0) return;

		// In customize, "clear" must keep the weapon line (same rule as the main row's
		// trash button) — wiping it would leave an empty, un-checkout-able configuration.
		if (TraderXUINavigationService.GetInstance().GetNavigationId() == ENavigationIds.CUSTOMIZE)
		{
			RemoveAllAttachmentsKeepMain();
			UpdateTotalPrice();
			UpdatePlayerMoney();
			MarkForCheck();
			return;
		}

		TraderXSelectionService.GetInstance().DeSelectAllItems();
		_inputCheckoutItems.Clear();
		UpdateTotalPrice();
		UpdatePlayerMoney();
		MarkForCheck();
	}

	// Build the single preset transaction for a customize purchase: the current main item
	// (variant) plus every selected attachment line, so it spawns assembled. Returns false
	// if there is no main item to buy.
	bool BuildCustomizeBuyTransaction(TraderXTransactionCollection transactions, int npcId)
	{
		TraderXProduct mainItem = CustomizeStateService.GetInstance().GetCurrentVariant();
		if (!mainItem) return false;

		array<string> attachments = new array<string>();
		foreach (TraderXProduct item : _inputCheckoutItems)
		{
			if (!item || CustomizeStateService.GetInstance().IsMainItem(item)) continue;
			int count = item.GetMultiplier();
			if (count < 1) count = 1;
			for (int i = 0; i < count; i++)
				attachments.Insert(item.productId);
		}

		TraderXPreset preset = TraderXPreset.CreateTraderXPreset("Custom", mainItem.productId, attachments);
		int mult = mainItem.GetMultiplier();
		if (mult < 1) mult = 1;
		int avgUnitPrice = TraderXPresetsService.GetInstance().CalculateProgressivePresetPrice(preset, mult) / mult;
		transactions.AddTransaction(TraderXTransaction.CreateBuyTransaction(mainItem, mult, avgUnitPrice, npcId, preset));
		return true;
	}

	void OnCheckoutClick()
	{
		TraderXInventoryManager.PlayMenuSound(ETraderXSounds.CONFIRM);
		if (_inputCheckoutItems.Count() == 0) return;
		if (TraderXTradingService.GetInstance().IsTransactionPending()) return;

		TraderXTransactionCollection transactions = new TraderXTransactionCollection();
		int npcId = TraderXTradingService.GetInstance().GetNpcId();

		// Customize buys the whole configuration as ONE preset transaction so the server
		// assembles the parts onto the item. The per-line loop below would instead buy each
		// attachment as a loose item — which drops vehicle parts at the player's feet.
		bool isCustomizeBuy = TraderXTradingService.GetInstance().GetTradeMode() == ETraderXTradeMode.BUY && TraderXUINavigationService.GetInstance().GetNavigationId() == ENavigationIds.CUSTOMIZE;

		// Category-licence gate: locked products can be browsed/customized but not bought.
		// Refuse with a toast naming the licence instead of sending a doomed request.
		if (IsBuyBlockedByLicense(isCustomizeBuy))
			return;

		if (isCustomizeBuy)
		{
			if (!BuildCustomizeBuyTransaction(transactions, npcId))
				return;
			SendTransactions(transactions, npcId);
			return;
		}

		map<string, int> soldSoFar = new map<string, int>();
		foreach (TraderXProduct item : _inputCheckoutItems)
		{
			if (TraderXTradingService.GetInstance().GetTradeMode() == ETraderXTradeMode.BUY)
			{
				int averageUnitPrice;
				TraderXTransaction buyTx;
				if (item.defaultPreset)
				{
					averageUnitPrice = TraderXPresetsService.GetInstance().CalculateProgressivePresetPrice(item.defaultPreset, item.GetMultiplier()) / item.GetMultiplier();
					buyTx = TraderXTransaction.CreateBuyTransaction(item, item.GetMultiplier(), averageUnitPrice, npcId, item.defaultPreset);
				}
				else
				{
					averageUnitPrice = TraderXPricingService.GetInstance().GetPricePreview(item.GetProductId(), true, item.GetMultiplier(), TraderXItemState.PRISTINE) / item.GetMultiplier();
					buyTx = TraderXTransaction.CreateBuyTransaction(item, item.GetMultiplier(), averageUnitPrice, npcId);
				}
				transactions.AddTransaction(buyTx);
			}
			else if (TraderXTradingService.GetInstance().GetTradeMode() == ETraderXTradeMode.SELL)
			{
				if (!item.GetPlayerItem())
					continue;
				// The server applies each sale to the stock in this order, so the Nth unit of a product
				// sent here is priced at stock + N - 1 (same productId key as the server's IncreaseStock).
				int sellPrice;
				TraderXProduct sellProduct = TraderXProductRepository.GetItemById(item.GetProductId());
				if (item.defaultPreset || !sellProduct)
				{
					sellPrice = TraderXPricingService.GetInstance().GetPricePreview(item.GetProductId(), false, 1, item.GetPlayerItem().healthLevel);
				}
				else
				{
					int soldBefore = 0;
					if (soldSoFar.Contains(item.GetProductId()))
						soldBefore = soldSoFar.Get(item.GetProductId());
					soldSoFar.Set(item.GetProductId(), soldBefore + 1);
					int sellStock = TraderXProductStockRepository.GetStockAmount(item.GetProductId()) + soldBefore;
					float stateMult = TraderXItemState.GetStateMultiplier(item.GetPlayerItem().healthLevel);
					sellPrice = TraderXPriceCalculation.CreateSellCalculation(sellProduct.sellPrice, sellProduct.coefficient, sellStock, 1, stateMult, sellProduct.IsStockUnlimited()).GetCalculatedPrice();
				}
				TraderXTransaction sellTx = TraderXTransaction.CreateSellTransaction(item, 1, sellPrice, npcId);
				transactions.AddTransaction(sellTx);
			}
		}

		SendTransactions(transactions, npcId);
	}

	private bool IsBuyBlockedByLicense(bool isCustomizeBuy)
	{
		if (TraderXTradingService.GetInstance().GetTradeMode() != ETraderXTradeMode.BUY)
			return false;

		TraderXLicenseService licenseService = TraderXLicenseService.GetInstance();
		// Customize: variants/attachments aren't listed in categories — gate on the base product.
		if (isCustomizeBuy)
			return licenseService.NotifyIfProductLocked(CustomizeStateService.GetInstance().GetOriginalItem());

		foreach (TraderXProduct cartItem : _inputCheckoutItems)
		{
			if (licenseService.NotifyIfProductLocked(cartItem))
				return true;
		}
		return false;
	}

	// Send a transaction collection to the server, remembering it so a NO_SPACE rejection can
	// be retried with ground-drop confirmed.
	private void SendTransactions(TraderXTransactionCollection transactions, int npcId)
	{
		m_PendingTransactions = transactions;
		m_PendingNpcId = npcId;
		TraderXTradingService.GetInstance().LockTransaction();
		GetRPCManager().SendRPC("TraderX", "GetTransactionsRequest", new Param2<TraderXTransactionCollection, int>(transactions, npcId));
	}

	// Narrow the pending collection to just the lines the server rejected for lack of space,
	// then ask the player whether to buy those anyway (items drop at their feet).
	void PromptGroundDropForNoSpace(TraderXTransactionResultCollection resultCollection)
	{
		if (!m_PendingTransactions)
			return;

		TraderXTransactionCollection retry = new TraderXTransactionCollection();
		foreach (TraderXTransaction pendingTx : m_PendingTransactions.GetAllTransactions())
		{
			if (pendingTx && IsNoSpaceProduct(resultCollection, pendingTx.GetProductId()))
				retry.AddTransaction(pendingTx);
		}

		if (retry.GetAllTransactions().Count() == 0)
			return;

		m_PendingTransactions = retry;
		ShowNoSpaceConfirm();
	}

	bool IsNoSpaceProduct(TraderXTransactionResultCollection resultCollection, string productId)
	{
		foreach (TraderXTransactionResult result : resultCollection.GetTransactionResults())
		{
			if (result && result.NeedsSpaceConfirm() && result.GetProductId() == productId)
				return true;
		}
		return false;
	}

	void ShowNoSpaceConfirm()
	{
		ModalConfirmationComponent.Event_OnConfirmClickCallBack.Insert(OnNoSpaceConfirm);
		ModalConfirmationComponent.Event_OnCancelClickCallBack.Insert(OnNoSpaceCancel);
		ModalConfirmationComponent.Open(Widget.TranslateString("#tpm_no_space_title"), Widget.TranslateString("#tpm_no_space_content"));
	}

	void OnNoSpaceConfirm()
	{
		CleanupNoSpaceModal();
		if (!m_PendingTransactions)
			return;

		foreach (TraderXTransaction pendingTx : m_PendingTransactions.GetAllTransactions())
		{
			if (pendingTx)
				pendingTx.SetAllowGroundDrop(true);
		}
		SendTransactions(m_PendingTransactions, m_PendingNpcId);
	}

	void OnNoSpaceCancel()
	{
		CleanupNoSpaceModal();
	}

	private void CleanupNoSpaceModal()
	{
		ModalConfirmationComponent.Event_OnConfirmClickCallBack.Remove(OnNoSpaceConfirm);
		ModalConfirmationComponent.Event_OnCancelClickCallBack.Remove(OnNoSpaceCancel);
		ModalConfirmationComponent.Close();
	}
}
