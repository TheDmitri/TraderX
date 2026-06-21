class SellPageComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("tx-sell-page", SellPageComponent);

	static ref ScriptInvoker Event_OnSellQuantityModeChanged = new ScriptInvoker();

	ref array<ref TraderXProduct> _inputSellProducts = new array<ref TraderXProduct>();
	ref map<int, ref array<ref TraderXProduct>> mPlayerItemsPerSlotId = new map<int, ref array<ref TraderXProduct>>();
	ref array<ref TraderXProduct> playerItemsFromSelectedNav = new array<ref TraderXProduct>();

	private static SellPageComponent m_Instance;

	bool entire_container_sell;
	bool selected_item_sell;
	string search_keyword;
	string total_quantity;
	string minimal_quantity;

	// List/grid view containers (toggled by TraderXViewModeService).
	Widget sellGridList;
	Widget sellGridTiles;

	// Sell-mode option buttons (highlighted to show the active mode).
	Widget entireContainerBtn;
	Widget selectItemsBtn;

	// Sell-quantity toggle: minimum needed vs the whole stack.
	Widget qtyMinBtn;
	Widget qtyAllBtn;
	bool m_SellAll;

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "tx-sell-page";
		m_Metadata.templatePath = "TraderX/datasets/gui/SellPage/SellPageView.layout";
	}

	static SellPageComponent GetInstance()
	{
		return m_Instance;
	}

	// Manual sell mode was removed (misleading / accidental sells). Kept for the sell
	// tooltip's call site; always false now.
	static bool IsManualSell()
	{
		return false;
	}

	override void OnInit()
	{
		m_Instance = this;
		AutoLoadWidgets();
		PlayerSlotBtnComponent.Event_OnPlayerSlotEventClickCallBack.Insert(OnPlayerSlotEventClick);
		ItemCardComponent.Event_OnItemCardClickEventCallBack.Insert(OnItemCardSelected);
		TraderXTradingService.Event_OnTraderXResponseReceived.Insert(OnTraderXResponseReceived);
		TraderXTradingService.GetInstance().SetMaxQuantity(false);
		TraderXVehicleParkingService.Event_OnVehicleParkingDataReceived.Insert(OnVehicleParkingDataReceived);
		TraderXViewModeService.Event_OnViewModeChangedCallBack.Insert(OnViewModeChanged);
		TraderXSearchBarComponent.Event_OnSearchChangedCallBack.Insert(OnSearchKeyword);
		Setup();
		TraderXViewModeBinder.Apply(sellGridList, sellGridTiles);
		UpdateQtyHighlight();
	}

	override void OnDestroy()
	{
		PlayerSlotBtnComponent.Event_OnPlayerSlotEventClickCallBack.Remove(OnPlayerSlotEventClick);
		ItemCardComponent.Event_OnItemCardClickEventCallBack.Remove(OnItemCardSelected);
		TraderXTradingService.Event_OnTraderXResponseReceived.Remove(OnTraderXResponseReceived);
		TraderXVehicleParkingService.Event_OnVehicleParkingDataReceived.Remove(OnVehicleParkingDataReceived);
		TraderXViewModeService.Event_OnViewModeChangedCallBack.Remove(OnViewModeChanged);
		TraderXSearchBarComponent.Event_OnSearchChangedCallBack.Remove(OnSearchKeyword);
		m_Instance = null;
	}

	void OnViewModeChanged(bool grid)
	{
		TraderXViewModeBinder.Apply(sellGridList, sellGridTiles);
	}

	void Setup()
	{
		TraderXUINavigationService.GetInstance().SetNavigationId(ENavigationIds.SELL);
		TraderXTradingService.GetInstance().SetTradeMode(ETraderXTradeMode.SELL);
		// The tx-checkout child mounts before this runs, so its OnInit read the stale
		// trade mode — re-sync the button/label now that SELL is set.
		if (CheckoutComponent.GetInstance()) CheckoutComponent.GetInstance().UpdateTradeModeText();
		InitPlayerItems();
		GetAllPlayerItems();
		FillItemCardList();
		SetSellMode();
	}

	// Default to "select items" — Manual mode was removed (misleading / accidental sells).
	void SetSellMode(int sellMode = ESellMode.SELECTED_ITEM)
	{
		UnSelectAllItems();
		entire_container_sell = false;
		selected_item_sell = false;
		switch (sellMode)
		{
			case ESellMode.ENTIRE_CONTAINER:
				entire_container_sell = true;
				SelectAllItems();
				break;
			case ESellMode.SELECTED_ITEM:
				selected_item_sell = true;
				break;
		}
		UpdateModeHighlight();
		MarkForCheck();
	}

	// Highlight the active sell-mode button: full-bright green accent, others dimmed.
	void UpdateModeHighlight()
	{
		SetModeBtnState(entireContainerBtn, entire_container_sell);
		SetModeBtnState(selectItemsBtn, selected_item_sell);
	}

	void SetModeBtnState(Widget btn, bool selected)
	{
		if (!btn) return;
		// SetColor multiplies the (dark) button style, so it can only darken — a gold
		// tint reads as muddy brown and looks *less* prominent than an inactive button.
		// Instead: active = full-bright warm (lit), inactive = dimmed/recessed.
		if (selected)
		{
			btn.SetColor(0xfff2e8cc); // warm cream at full brightness = active
			btn.SetAlpha(1.0);
		}
		else
		{
			btn.SetColor(0xff595e66); // dim slate
			btn.SetAlpha(0.5);
		}
	}

	void UnSelectAllItems()
	{
		TraderXSelectionService.GetInstance().DeSelectAllItems();
	}

	void SelectAllItems()
	{
		foreach (TraderXProduct item : _inputSellProducts)
		{
			if (!item) continue;
			TraderXSelectionService.GetInstance().SelectItem(item);
		}
		MarkForCheck();
	}

	void InitPlayerItems()
	{
		PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
		if (!player) return;
		mPlayerItemsPerSlotId.Clear();
		foreach (int slotId : sAttachmentSlots)
		{
			mPlayerItemsPerSlotId.Set(slotId, new array<ref TraderXProduct>());
			array<ref TraderXProduct> items = mPlayerItemsPerSlotId[slotId];
			TraderXTradingService.GetInstance().GetTraderXProductsFromSlotId(player, slotId, items);
			PlayerSlotBtnComponent btn = PlayerSlotBtnComponent.GetPlayerSlotsButtonById(EPlayerSlotNavigationButton.GetNavBtnFromSlotId(slotId));
			if (btn) btn.SetItemCount(items.Count());
		}
		InitVehiclesInParkingSpots();
	}

	void InitVehiclesInParkingSpots()
	{
		mPlayerItemsPerSlotId.Set(EPlayerSlotNavigationButton.VEHICLES, new array<ref TraderXProduct>());
		RequestVehicleParkingData();
	}

	void RequestVehicleParkingData()
	{
		int currentTraderId = TraderXTradingService.GetInstance().GetNpcId();
		if (currentTraderId == -1) return;
		GetRPCManager().SendRPC("TraderX", "GetVehicleParkingDataRequest", new Param1<int>(currentTraderId), true, null);
	}

	void OnVehicleParkingDataReceived(array<ref TraderXProduct> vehicleProducts)
	{
		if (mPlayerItemsPerSlotId.Contains(EPlayerSlotNavigationButton.VEHICLES))
			mPlayerItemsPerSlotId.Set(EPlayerSlotNavigationButton.VEHICLES, vehicleProducts);
		PlayerSlotBtnComponent btn = PlayerSlotBtnComponent.GetPlayerSlotsButtonById(EPlayerSlotNavigationButton.VEHICLES);
		if (btn) btn.SetItemCount(vehicleProducts.Count());

		PlayerSlotBtnComponent selectedBtn = PlayerSlotNavComponent.GetNavigationInstance().GetSelectedNavBtn();
		if (selectedBtn && selectedBtn.navBtnId == EPlayerSlotNavigationButton.VEHICLES)
		{
			playerItemsFromSelectedNav = vehicleProducts;
			FillItemCardList();
		}
		if (selectedBtn && selectedBtn.navBtnId == EPlayerSlotNavigationButton.ALL)
		{
			GetAllPlayerItems();
			FillItemCardList();
		}
	}

	void GetAllPlayerItems()
	{
		playerItemsFromSelectedNav = new array<ref TraderXProduct>();
		foreach (int slotId : sAttachmentSlots)
		{
			PlayerSlotBtnComponent slotBtn = PlayerSlotBtnComponent.GetPlayerSlotsButtonById(EPlayerSlotNavigationButton.GetNavBtnFromSlotId(slotId));
			if (slotBtn && slotBtn.IsLocked()) continue;
			foreach (TraderXProduct item : mPlayerItemsPerSlotId.Get(slotId))
				playerItemsFromSelectedNav.Insert(item);
		}
		PlayerSlotBtnComponent vehBtn = PlayerSlotBtnComponent.GetPlayerSlotsButtonById(EPlayerSlotNavigationButton.VEHICLES);
		if (vehBtn && !vehBtn.IsLocked() && mPlayerItemsPerSlotId.Contains(EPlayerSlotNavigationButton.VEHICLES))
		{
			foreach (TraderXProduct vehicle : mPlayerItemsPerSlotId.Get(EPlayerSlotNavigationButton.VEHICLES))
				playerItemsFromSelectedNav.Insert(vehicle);
		}
		PlayerSlotBtnComponent allBtn = PlayerSlotBtnComponent.GetPlayerSlotsButtonById(EPlayerSlotNavigationButton.GetNavBtnFromSlotId(EPlayerSlotNavigationButton.ALL));
		if (allBtn) allBtn.SetItemCount(playerItemsFromSelectedNav.Count());
	}

	void FillItemCardList()
	{
		_inputSellProducts.Clear();
		foreach (TraderXProduct item : playerItemsFromSelectedNav)
		{
			if (!item) continue;
			_inputSellProducts.Insert(item);
		}
		MarkForCheck();
	}

	void OnPlayerSlotEventClick(PlayerSlotBtnComponent buttonComp, int command)
	{
		if (!buttonComp) return;
		if (command == ETraderXClickEvents.LCLICK)
		{
			int slotId = EPlayerSlotNavigationButton.GetInventoryIdFromSlotId(buttonComp.navBtnId);
			if (slotId == EPlayerSlotNavigationButton.ALL)
				GetAllPlayerItems();
			else if (buttonComp.navBtnId == EPlayerSlotNavigationButton.VEHICLES)
			{
				InitVehiclesInParkingSpots();
				if (mPlayerItemsPerSlotId.Contains(EPlayerSlotNavigationButton.VEHICLES))
					playerItemsFromSelectedNav = mPlayerItemsPerSlotId[EPlayerSlotNavigationButton.VEHICLES];
			}
			else
			{
				if (mPlayerItemsPerSlotId[slotId])
					playerItemsFromSelectedNav = mPlayerItemsPerSlotId[slotId];
			}
			FillItemCardList();
			// In "Sell Container" mode, every item in the freshly selected slot must
			// be auto-selected — clear the old slot's selection first, then select all.
			if (entire_container_sell)
			{
				UnSelectAllItems();
				SelectAllItems();
			}
		}
		if (command == ETraderXClickEvents.RCLICK)
		{
			PlayerSlotBtnComponent selectedBtn = PlayerSlotNavComponent.GetNavigationInstance().GetSelectedNavBtn();
			if (!selectedBtn) return;
			if (selectedBtn.navBtnId == buttonComp.navBtnId)
			{
				_inputSellProducts.Clear();
				MarkForCheck();
				return;
			}
			if (selectedBtn.navBtnId == EPlayerSlotNavigationButton.ALL)
			{
				GetAllPlayerItems();
				FillItemCardList();
			}
		}
	}

	void OnItemCardSelected(ItemCardComponent itemSelected, int command)
	{
		if (TraderXProductStockRepository.IsStockReached(itemSelected.GetItem().productId, itemSelected.GetItem().maxStock)) return;
		switch (command)
		{
			case ETraderXClickEvents.LCLICK:
				OnItemCardLClick(itemSelected);
				break;
			case ETraderXClickEvents.RCLICK:
				OnItemCardRClick(itemSelected);
				break;
			// Double-click-to-sell removed with Manual mode (accidental sells).
			case ETraderXClickEvents.SHIFT_LCLICK:
				OnItemCardShiftLClick(itemSelected);
				break;
			case ETraderXClickEvents.SHIFT_RCLICK:
				break;
		}
	}

	void OnItemCardLClick(ItemCardComponent itemSelected)
	{
		// Titles live on the toggle pills now — bind just the numbers.
		total_quantity = itemSelected.itemVolume.ToString();
		minimal_quantity = TraderXTradeQuantity.GetItemSellQuantity(itemSelected._inputProduct.className, itemSelected._inputProduct.tradeQuantity).ToString();
		MarkForCheck();
	}

	void OnItemCardRClick(ItemCardComponent itemSelected)
	{
		if (!selected_item_sell) return;
		// TODO: select all items of same type via QueryChildren
	}

	void OnItemCardShiftLClick(ItemCardComponent itemShiftLClicked)
	{
		// TODO: select item + children via QueryChildren
	}

	void OnSellEntireContainerClick()
	{
		SetSellMode(ESellMode.ENTIRE_CONTAINER);
	}

	void OnSelectItemsClick()
	{
		SetSellMode(ESellMode.SELECTED_ITEM);
	}

	void OnQtyMinClick()
	{
		SetQuantityMode(false);
	}

	void OnQtyAllClick()
	{
		SetQuantityMode(true);
	}

	// Binary choice: sell the minimum trade unit per item, or the whole stack.
	void SetQuantityMode(bool sellAll)
	{
		m_SellAll = sellAll;
		TraderXTradingService.GetInstance().SetMaxQuantity(sellAll);
		Event_OnSellQuantityModeChanged.Invoke(sellAll);
		UpdateQtyHighlight();
		TraderXInventoryManager.PlayMenuSound(ETraderXSounds.QUICK_EVENT);
	}

	void UpdateQtyHighlight()
	{
		SetModeBtnState(qtyMinBtn, !m_SellAll);
		SetModeBtnState(qtyAllBtn, m_SellAll);
	}

	// Debounced keyword pushed up from the embedded tx-search-bar component.
	void OnSearchKeyword(string keyword)
	{
		search_keyword = keyword;
		// TODO: filter visible items
	}

	void OnTraderXResponseReceived(int responseReceived, TraderXTransactionResultCollection transactionResultCollection = null)
	{
		if (responseReceived == ETraderXResponse.ALL_STOCK_RECEIVED || responseReceived == ETraderXResponse.TRANSACTIONS)
			Refresh();
	}

	void Refresh()
	{
		_inputSellProducts.Clear();
		CheckoutComponent checkoutComp = CheckoutComponent.GetInstance();
		if (checkoutComp) checkoutComp.UpdateCheckoutView();
		InitPlayerItems();
		PlayerSlotBtnComponent selectedBtn = PlayerSlotNavComponent.GetNavigationInstance().GetSelectedNavBtn();
		if (selectedBtn) OnPlayerSlotEventClick(selectedBtn, ETraderXClickEvents.LCLICK);
	}
}
