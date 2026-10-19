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

	void UpdateTradeModeText()
	{
		if (TraderXTradingService.GetInstance() && TraderXTradingService.GetInstance().GetTradeMode() == ETraderXTradeMode.SELL)
		{
			checkout_label = "#tpm_income";
			checkout_btn_text = "#tpm_sell";
		}
		else
		{
			checkout_label = "#tpm_to_pay";
			checkout_btn_text = "#tpm_purchase";
		}
		MarkForCheck();
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
		int playerMoney = TraderXTradingService.GetInstance().GetPlayerMoneyAmount(player);
		int bankBalance = 0;
		if (TraderXProviderRegistry.GetInstance().HasBanking())
		{
			ITraderXPaymentProvider bankProv = TraderXProviderRegistry.GetInstance().GetBankingProvider();
			if (bankProv && bankProv.IsAvailable(player))
				bankBalance = bankProv.GetClientBalance();
		}
		bank_amount = TraderXQuantityManager.GetFormattedMoneyAmount(bankBalance);
		money_amount = TraderXQuantityManager.GetFormattedMoneyAmount(playerMoney + bankBalance);
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
		total_amount = TraderXQuantityManager.GetFormattedMoneyAmount(totalPrice);
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

		TraderXPreset preset = TraderXPreset.CreateTraderXPreset("", mainItem.productId, attachments);
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
		if (isCustomizeBuy)
		{
			if (!BuildCustomizeBuyTransaction(transactions, npcId))
				return;
			SendTransactions(transactions, npcId);
			return;
		}

		foreach (TraderXProduct item : _inputCheckoutItems)
		{
			if (TraderXTradingService.GetInstance().GetTradeMode() == ETraderXTradeMode.BUY)
			{
				int averageUnitPrice;
				if (item.defaultPreset)
				{
					averageUnitPrice = TraderXPresetsService.GetInstance().CalculateProgressivePresetPrice(item.defaultPreset, item.GetMultiplier()) / item.GetMultiplier();
					transactions.AddTransaction(TraderXTransaction.CreateBuyTransaction(item, item.GetMultiplier(), averageUnitPrice, npcId, item.defaultPreset));
				}
				else
				{
					averageUnitPrice = TraderXPricingService.GetInstance().GetPricePreview(item.GetProductId(), true, item.GetMultiplier(), TraderXItemState.PRISTINE) / item.GetMultiplier();
					transactions.AddTransaction(TraderXTransaction.CreateBuyTransaction(item, item.GetMultiplier(), averageUnitPrice, npcId));
				}
			}
			else if (TraderXTradingService.GetInstance().GetTradeMode() == ETraderXTradeMode.SELL)
			{
				int sellPrice = TraderXPricingService.GetInstance().GetPricePreview(item.GetProductId(), false, 1, item.GetPlayerItem().healthLevel);
				transactions.AddTransaction(TraderXTransaction.CreateSellTransaction(item, 1, sellPrice, npcId));
			}
		}

		SendTransactions(transactions, npcId);
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
