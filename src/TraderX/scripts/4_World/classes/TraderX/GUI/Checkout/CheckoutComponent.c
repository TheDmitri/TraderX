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

	private static ref CheckoutComponent m_Instance;

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
					MarkForCheck();
					return;
				}
			}
			else
			{
				if (item.productId == _inputCheckoutItems[i].productId)
				{
					_inputCheckoutItems.Remove(i);
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
		money_amount = TraderXQuantityManager.GetFormattedMoneyAmount(playerMoney);
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
		MarkForCheck();
	}

	void OnTraderXResponseReceived(int responseReceived, TraderXTransactionResultCollection transactionResultCollection = null)
	{
		if (responseReceived != ETraderXResponse.TRANSACTIONS || !transactionResultCollection) return;

		array<UUID> itemsToDeselect = new array<UUID>();
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

	void OnCheckoutClick()
	{
		TraderXInventoryManager.PlayMenuSound(ETraderXSounds.CONFIRM);
		if (_inputCheckoutItems.Count() == 0) return;
		if (TraderXTradingService.GetInstance().IsTransactionPending()) return;

		TraderXTransactionCollection transactions = new TraderXTransactionCollection();
		int npcId = TraderXTradingService.GetInstance().GetNpcId();

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

		TraderXTradingService.GetInstance().LockTransaction();
		GetRPCManager().SendRPC("TraderX", "GetTransactionsRequest", new Param2<TraderXTransactionCollection, int>(transactions, npcId));
	}
}
