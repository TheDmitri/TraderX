class SellPageViewController: ViewController
{
    static ref ScriptInvoker Event_OnSliderQuantityChangedCallBack = new ScriptInvoker();

    ref ObservableCollection<ref ItemCardView> item_card_list = new ObservableCollection<ref ItemCardView>(this);

    ref map<int, ref array<ref TraderXProduct>> mPlayerItemsPerSlotId = new map<int, ref array<ref TraderXProduct>>();

    ref array<ref TraderXProduct> playerItemsFromSelectedNav = new array<ref TraderXProduct>();

    private static SellPageViewController m_Instance;

    static SellPageViewController GetInstance() {
        return m_Instance;
    }

    bool entire_container_sell;
    bool selected_item_sell;
    bool manual_sell;

    string search_keyword;

    string total_quantity;
    string minimal_quantity;

    float slider_quantity;
    
    void SellPageViewController()
    {
        m_Instance = this;
        GetTraderXLogger().LogDebug("SellPageViewController");
        PlayerSlotNavigationButtonViewController.Event_OnPlayerSlotEventClickCallBack.Insert(OnPlayerSlotEventClick);
        ItemCardViewController.Event_OnItemCardClickEventCallBack.Insert(OnItemCardSelected);
        TraderXTradingService.Event_OnTraderXResponseReceived.Insert(OnTraderXResponseReceived);
        TraderXTradingService.GetInstance().SetMaxQuantity(false);
        TraderXVehicleParkingService.Event_OnVehicleParkingDataReceived.Insert(OnVehicleParkingDataReceived);
    }

    static bool IsManualSell() {
        if (GetInstance())
            return GetInstance().manual_sell;
        return false;
    }

    void Setup()
    {
        TraderXUINavigationService.GetInstance().SetNavigationId(ENavigationIds.SELL);
        TraderXTradingService.GetInstance().SetTradeMode(ETraderXTradeMode.SELL);
        InitPlayerItems();
        GetAllPlayerItems();
        FillItemCardList();
        SetSellMode();
    }

    void SetSellMode(int sellMode = ESellMode.MANUAL)
    {
        UnSelectAllItems();
        entire_container_sell = false;
        selected_item_sell = false;
        manual_sell = false;
        
        switch(sellMode)
        {
            case ESellMode.MANUAL:
                manual_sell = true;
                break;
            case ESellMode.ENTIRE_CONTAINER:
                entire_container_sell = true;
                SelectAllItems();
                break;
            case ESellMode.SELECTED_ITEM:
                selected_item_sell = true;
                break;
        }

        NotifyPropertiesChanged({"entire_container_sell", "selected_item_sell", "manual_sell"}, false);
    }

    void UnSelectAllItems() 
    {
        for(int i = 0; i < item_card_list.GetArray().Count(); i++)
        {
            item_card_list.Get(i).GetTemplateController().SetItemSelected(false);
        }
    }

    void SelectAllItems()
    {
        for(int i = 0; i < item_card_list.GetArray().Count(); i++)
        {
            item_card_list.Get(i).GetTemplateController().SetItemSelected(true);
        }
    }

    void InitPlayerItems()
    {
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if(!player)
            return;

        mPlayerItemsPerSlotId.Clear();
        foreach(int slotId: sAttachmentSlots)
        {
            mPlayerItemsPerSlotId.Set(slotId, new array<ref TraderXProduct>());
            array<ref TraderXProduct> items = mPlayerItemsPerSlotId[slotId];
            TraderXTradingService.GetInstance().GetTraderXProductsFromSlotId(player, slotId, items);

            PlayerSlotNavigationButtonViewController.GetPlayerSlotsButtonsControllerById(EPlayerSlotNavigationButton.GetNavBtnFromSlotId(slotId)).SetItemCount(items.Count());
        }

        // Initialize vehicles in parking spots
        InitVehiclesInParkingSpots();
    }

    void InitVehiclesInParkingSpots()
    {
        // Initialize empty vehicles slot
        mPlayerItemsPerSlotId.Set(EPlayerSlotNavigationButton.VEHICLES, new array<ref TraderXProduct>());
        
        // Request vehicle data from server via RPC
        RequestVehicleParkingData();
    }

    void RequestVehicleParkingData()
    {
        int currentTraderId = TraderXTradingService.GetInstance().GetNpcId();
        if (currentTraderId == -1) {
            GetTraderXLogger().LogWarning("No current trader set for vehicle detection");
            return;
        }

        // Send RPC request to server for vehicle parking data
        GetRPCManager().SendRPC("TraderX", "GetVehicleParkingDataRequest", new Param1<int>(currentTraderId), true, null);
    }

    void OnVehicleParkingDataReceived(array<ref TraderXProduct> vehicleProducts)
    {
        // Update the vehicles slot with received data
        if (mPlayerItemsPerSlotId.Contains(EPlayerSlotNavigationButton.VEHICLES)) {
            mPlayerItemsPerSlotId.Set(EPlayerSlotNavigationButton.VEHICLES, vehicleProducts);
        }
        
        // Update vehicle navigation button count
        PlayerSlotNavigationButtonViewController.GetPlayerSlotsButtonsControllerById(EPlayerSlotNavigationButton.VEHICLES).SetItemCount(vehicleProducts.Count());
        
        // Refresh the UI if vehicles tab is currently selected
        PlayerSlotNavigationButtonViewController selectedNavBtn = PlayerSlotNavigationViewController.GetNavigationInstance().GetSelectedNavBtn();
        if (selectedNavBtn && selectedNavBtn.navBtnId == EPlayerSlotNavigationButton.VEHICLES) {
            playerItemsFromSelectedNav = vehicleProducts;
            FillItemCardList();
        }
        
        // Also update "All" count if it includes vehicles
        if (selectedNavBtn && selectedNavBtn.navBtnId == EPlayerSlotNavigationButton.ALL) {
            GetAllPlayerItems();
            FillItemCardList();
        }
    }

    void GetAllPlayerItems()
    {
        GetTraderXLogger().LogDebug("GetAllPlayerItems");
        playerItemsFromSelectedNav = new array<ref TraderXProduct>();

        foreach(int slotId: sAttachmentSlots)
        {
            if(PlayerSlotNavigationButtonViewController.GetPlayerSlotsButtonsControllerById(EPlayerSlotNavigationButton.GetNavBtnFromSlotId(slotId)).IsLocked())
                continue;
            
            foreach(TraderXProduct item: mPlayerItemsPerSlotId.Get(slotId))
            {
                playerItemsFromSelectedNav.Insert(item);
            }
        }

        // Include vehicles if not locked
        if(!PlayerSlotNavigationButtonViewController.GetPlayerSlotsButtonsControllerById(EPlayerSlotNavigationButton.VEHICLES).IsLocked())
        {
            if(mPlayerItemsPerSlotId.Contains(EPlayerSlotNavigationButton.VEHICLES))
            {
                foreach(TraderXProduct vehicle: mPlayerItemsPerSlotId.Get(EPlayerSlotNavigationButton.VEHICLES))
                {
                    playerItemsFromSelectedNav.Insert(vehicle);
                }
            }
        }

        PlayerSlotNavigationButtonViewController.GetPlayerSlotsButtonsControllerById(EPlayerSlotNavigationButton.GetNavBtnFromSlotId(EPlayerSlotNavigationButton.ALL)).SetItemCount(playerItemsFromSelectedNav.Count());
    }

    void FillItemCardList()
    {
        GetTraderXLogger().LogDebug("FillItemCardList");
        item_card_list.Clear();
        foreach(TraderXProduct item: playerItemsFromSelectedNav)
        {
            if(!item)
                continue;

            item_card_list.Insert(new ItemCardView(item, EItemCardSize.LARGE, true, false));
        }

        if(search_keyword != string.Empty)
            FilterItemList();
    }

    void OnPlayerSlotEventClick(PlayerSlotNavigationButtonViewController buttonViewController, int command = ETraderXClickEvents.LCLICK)
    {
        if(!buttonViewController || buttonViewController && !buttonViewController.navBtnId)
            return;

        if(command == ETraderXClickEvents.LCLICK)
        {
            int slotId = EPlayerSlotNavigationButton.GetInventoryIdFromSlotId(buttonViewController.navBtnId);
            
            if(slotId == EPlayerSlotNavigationButton.ALL){
                GetAllPlayerItems();
            }
            else if(buttonViewController.navBtnId == EPlayerSlotNavigationButton.VEHICLES){
                // Refresh vehicle list to get current parking spot status
                InitVehiclesInParkingSpots();
                if(mPlayerItemsPerSlotId.Contains(EPlayerSlotNavigationButton.VEHICLES)){
                    playerItemsFromSelectedNav = mPlayerItemsPerSlotId[EPlayerSlotNavigationButton.VEHICLES];
                }
            }
            else{
                if(mPlayerItemsPerSlotId[slotId]){
                    playerItemsFromSelectedNav = mPlayerItemsPerSlotId[slotId];
                }
            }
            FillItemCardList();
        }

        if(command == ETraderXClickEvents.RCLICK)
        {
            PlayerSlotNavigationButtonViewController selectedNavBtn = PlayerSlotNavigationViewController.GetNavigationInstance().GetSelectedNavBtn();
            if(!selectedNavBtn)
                return;

            if(selectedNavBtn.navBtnId == buttonViewController.navBtnId){
                item_card_list.Clear();
                return;
            }

            if(selectedNavBtn.navBtnId == EPlayerSlotNavigationButton.ALL)
            {
                GetAllPlayerItems();
                FillItemCardList();
            }
        }
    }

    void OnItemCardSelected(ItemCardViewController itemSelected, int command = ETraderXClickEvents.LCLICK)
    {
        if(TraderXProductStockRepository.IsStockReached(itemSelected.GetItem().productId, itemSelected.GetItem().maxStock))
            return;

        switch(command)
        {
            case ETraderXClickEvents.LCLICK:
                OnItemCardLClick(itemSelected);
            break;

            case ETraderXClickEvents.RCLICK:
                OnItemCardRClick(itemSelected);
            break;
                
            case ETraderXClickEvents.DOUBLE_CLICK:
                OnItemCardDoubleClick(itemSelected);
            break;

            case ETraderXClickEvents.SHIFT_LCLICK:
                OnItemCardShiftLClick(itemSelected);
            break;

            case ETraderXClickEvents.SHIFT_RCLICK:
                OnItemCardShiftRClick(itemSelected);
            break;

            default:
                GetTraderXLogger().LogWarning("Unknown item card click event");
                break;
        }
    }

    void OnItemCardLClick(ItemCardViewController itemSelected)
    {
        int volume;
        itemSelected.GetItemVolume(volume);
        total_quantity = "Total Quantity: " + volume.ToString();
        minimal_quantity = "Minimal Needed: " + TraderXTradeQuantity.GetItemSellQuantity(itemSelected.item.className, itemSelected.item.tradeQuantity).ToString();
        NotifyPropertiesChanged({"total_quantity", "minimal_quantity"});
    }

    void OnItemCardRClick(ItemCardViewController itemSelected)
    {
        if(!selected_item_sell)
            return;

        for(int i = 0; i < item_card_list.GetArray().Count(); i++)
        {
            ItemCardViewController itemCard = item_card_list.Get(i).GetTemplateController();
            if(!itemCard)
                continue;

            if(itemCard.preview.GetType() == itemSelected.preview.GetType())
                itemCard.SetItemSelected(true);
        }
    }   

    void OnItemCardDoubleClick(ItemCardViewController itemDoubleClicked)
    {
        if(!manual_sell)
            return;

        int npcId = TraderXTradingService.GetInstance().GetNpcId();
        int sellPrice = itemDoubleClicked.GetPrice();
        
        TraderXTransaction sellTransaction = TraderXTransaction.CreateSellTransaction(itemDoubleClicked.item, 1, sellPrice, npcId);
        
        TraderXTransactionCollection transactionCollection = new TraderXTransactionCollection();
        transactionCollection.AddTransaction(sellTransaction);
        
        if (transactionCollection.IsEmpty()) {
            GetTraderXLogger().LogError("OnItemCardDoubleClick - Transaction collection is empty after adding transaction!");
            return;
        }
        
        GetRPCManager().SendRPC("TraderX", "GetTransactionsRequest", new Param2<TraderXTransactionCollection, int>(transactionCollection, npcId));
    }

    void OnItemCardShiftLClick(ItemCardViewController itemShiftLClicked)
    {
        if(manual_sell)
            return;

        array<EntityAI> entityItems = new array<EntityAI>();
        TraderXInventoryManager.GetEntitiesChildren(itemShiftLClicked.preview, entityItems);

        itemShiftLClicked.SetItemSelected(true);

        for(int i = 0; i < item_card_list.GetArray().Count(); i++)
        {
            ItemCardViewController itemCard = item_card_list.Get(i).GetTemplateController();
            if(!itemCard)
                continue;

            if(entityItems.Find(itemCard.preview) != -1 && itemCard.selectable)
                itemCard.SetItemSelected(true);
        }
    }

    void OnItemCardShiftRClick(ItemCardViewController itemShiftRClicked)
    {
        if(manual_sell)
            return;
    }

    bool OnSellEntireContainerCheck(ButtonCommandArgs args)
    {
        SetSellMode(ESellMode.ENTIRE_CONTAINER);
        return true;
    }

    bool OnSelectItemsCheck(ButtonCommandArgs args)
    {
        SetSellMode(ESellMode.SELECTED_ITEM);
        return true;
    }

    bool OnManualCheck(ButtonCommandArgs args)
    {
        SetSellMode(ESellMode.MANUAL);
        return true;
    }

    void OnTraderXResponseReceived(int responseReceived, TraderXTransactionResultCollection transactionResultCollection = null)
    { 
        if(responseReceived == ETraderXResponse.ALL_STOCK_RECEIVED)
        { 
            for(int i = 0; i < item_card_list.GetArray().Count(); i++)
            {
                ItemCardViewController itemCardViewController = item_card_list.Get(i).GetTemplateController();
                itemCardViewController.UpdateStock();                
            }
        }

        if(responseReceived == ETraderXResponse.TRANSACTIONS)
        { 
            Refresh();
            TraderXInventoryManager.PlayMenuSound(ETraderXSounds.COINS);
        }
    }

    void Refresh()
    {
        item_card_list.Clear();
        CheckoutViewController.GetInstance().UpdateCheckoutView();
        InitPlayerItems();
        OnPlayerSlotEventClick(PlayerSlotNavigationViewController.GetNavigationInstance().GetSelectedNavBtn());
    }

    void FilterItemList()
    {
        for(int i = 0; i < item_card_list.GetArray().Count(); i++)
        {
            TraderXProduct item = item_card_list.Get(i).GetTemplateController().item;
            if(item.className.Contains(search_keyword) || item.GetDisplayName().Contains(search_keyword) || search_keyword == string.Empty){
                item_card_list.Get(i).Show(true);
            }else{
                item_card_list.Get(i).Show(false);
            }
        }
    }

    override void PropertyChanged(string property_name)
	{
		switch (property_name)
		{
			case "search_keyword": 
			{
				FilterItemList();
				break;
			}

            case "slider_quantity":
            {
                TraderXTradingService.GetInstance().SetMaxQuantity(slider_quantity == 1.0);
                Event_OnSliderQuantityChangedCallBack.Invoke(slider_quantity == 1.0);
                break;
            }
		}
	}
}