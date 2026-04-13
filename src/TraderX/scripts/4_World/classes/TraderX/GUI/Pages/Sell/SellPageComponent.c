class SellPageComponent extends DZlrComponentBase
{
    private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("traderx-sell-page", SellPageComponent);

    static ref ScriptInvoker Event_OnSliderQuantityChangedCallBack = new ScriptInvoker();

    ref array<ref ItemCardComponent> m_ItemCards = new array<ref ItemCardComponent>();
    ref map<int, ref array<ref TraderXProduct>> mPlayerItemsPerSlotId = new map<int, ref array<ref TraderXProduct>>();
    ref array<ref TraderXProduct> playerItemsFromSelectedNav = new array<ref TraderXProduct>();

    private static SellPageComponent m_Instance;

    bool entire_container_sell;
    bool selected_item_sell;
    bool manual_sell;
    string search_keyword;
    string total_quantity;
    string minimal_quantity;
    float slider_quantity;

    // Widgets
    Widget item_card_list;
    EditBoxWidget searchBox;
    TextWidget totalQuantityText, minimalQuantityText;
    ButtonWidget entireContainerBtn, selectedItemBtn, manualBtn;
    SliderWidget quantitySlider;

    static SellPageComponent GetInstance()
    {
        return m_Instance;
    }

    static bool IsManualSell()
    {
        if (GetInstance())
            return GetInstance().manual_sell;
        return false;
    }

    override protected void RegisterMetadata()
    {
        m_Metadata = new ComponentMetadata();
        m_Metadata.selector = "traderx-sell-page";
        m_Metadata.templatePath = "TraderX/datasets/gui/SellPage/SellPageView.layout";
    }

    override void OnInit()
    {
        m_Instance = this;
        PlayerSlotNavigationButtonComponent.Event_OnPlayerSlotEventClickCallBack.Insert(OnPlayerSlotEventClick);
        ItemCardComponent.Event_OnItemCardClickEventCallBack.Insert(OnItemCardSelected);
        TraderXTradingService.Event_OnTraderXResponseReceived.Insert(OnTraderXResponseReceived);
        TraderXTradingService.GetInstance().SetMaxQuantity(false);
        TraderXVehicleParkingService.Event_OnVehicleParkingDataReceived.Insert(OnVehicleParkingDataReceived);
        Setup();
    }

    override void OnDestroy()
    {
        PlayerSlotNavigationButtonComponent.Event_OnPlayerSlotEventClickCallBack.Remove(OnPlayerSlotEventClick);
        ItemCardComponent.Event_OnItemCardClickEventCallBack.Remove(OnItemCardSelected);
        TraderXTradingService.Event_OnTraderXResponseReceived.Remove(OnTraderXResponseReceived);
        TraderXVehicleParkingService.Event_OnVehicleParkingDataReceived.Remove(OnVehicleParkingDataReceived);
        ClearItemCards();
        m_Instance = null;
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

        switch (sellMode)
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
    }

    void UnSelectAllItems()
    {
        foreach (ItemCardComponent card : m_ItemCards)
        {
            card.SetItemSelected(false);
        }
    }

    void SelectAllItems()
    {
        foreach (ItemCardComponent card : m_ItemCards)
        {
            card.SetItemSelected(true);
        }
    }

    void InitPlayerItems()
    {
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player)
            return;

        mPlayerItemsPerSlotId.Clear();
        foreach (int slotId : sAttachmentSlots)
        {
            mPlayerItemsPerSlotId.Set(slotId, new array<ref TraderXProduct>());
            array<ref TraderXProduct> items = mPlayerItemsPerSlotId[slotId];
            TraderXTradingService.GetInstance().GetTraderXProductsFromSlotId(player, slotId, items);
            PlayerSlotNavigationButtonComponent.GetPlayerSlotsButtonsControllerById(EPlayerSlotNavigationButton.GetNavBtnFromSlotId(slotId)).SetItemCount(items.Count());
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
        if (currentTraderId == -1)
            return;

        GetDayZularRPCManager().SendRPC("TraderX", "GetVehicleParkingDataRequest", new Param1<int>(currentTraderId), true, null);
    }

    void OnVehicleParkingDataReceived(array<ref TraderXProduct> vehicleProducts)
    {
        if (mPlayerItemsPerSlotId.Contains(EPlayerSlotNavigationButton.VEHICLES))
            mPlayerItemsPerSlotId.Set(EPlayerSlotNavigationButton.VEHICLES, vehicleProducts);

        PlayerSlotNavigationButtonComponent.GetPlayerSlotsButtonsControllerById(EPlayerSlotNavigationButton.VEHICLES).SetItemCount(vehicleProducts.Count());

        PlayerSlotNavigationButtonComponent selectedNavBtn = PlayerSlotNavigationComponent.GetNavigationInstance().GetSelectedNavBtn();
        if (selectedNavBtn && selectedNavBtn.navBtnId == EPlayerSlotNavigationButton.VEHICLES)
        {
            playerItemsFromSelectedNav = vehicleProducts;
            FillItemCardList();
        }

        if (selectedNavBtn && selectedNavBtn.navBtnId == EPlayerSlotNavigationButton.ALL)
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
            if (PlayerSlotNavigationButtonComponent.GetPlayerSlotsButtonsControllerById(EPlayerSlotNavigationButton.GetNavBtnFromSlotId(slotId)).IsLocked())
                continue;

            foreach (TraderXProduct item : mPlayerItemsPerSlotId.Get(slotId))
            {
                playerItemsFromSelectedNav.Insert(item);
            }
        }

        if (!PlayerSlotNavigationButtonComponent.GetPlayerSlotsButtonsControllerById(EPlayerSlotNavigationButton.VEHICLES).IsLocked())
        {
            if (mPlayerItemsPerSlotId.Contains(EPlayerSlotNavigationButton.VEHICLES))
            {
                foreach (TraderXProduct vehicle : mPlayerItemsPerSlotId.Get(EPlayerSlotNavigationButton.VEHICLES))
                {
                    playerItemsFromSelectedNav.Insert(vehicle);
                }
            }
        }

        PlayerSlotNavigationButtonComponent.GetPlayerSlotsButtonsControllerById(EPlayerSlotNavigationButton.GetNavBtnFromSlotId(EPlayerSlotNavigationButton.ALL)).SetItemCount(playerItemsFromSelectedNav.Count());
    }

    void ClearItemCards()
    {
        foreach (ItemCardComponent card : m_ItemCards)
        {
            if (card && card.GetRoot())
                card.GetRoot().Unlink();
        }
        m_ItemCards.Clear();
    }

    void FillItemCardList()
    {
        ClearItemCards();
        foreach (TraderXProduct item : playerItemsFromSelectedNav)
        {
            if (!item)
                continue;

            ItemCardComponent card = ItemCardComponent.CreateItemCard(item, EItemCardSize.LARGE, ETraderXCategoryType.NONE, true, false);
            if (card && item_card_list)
            {
                item_card_list.AddChild(card.GetRoot());
                card.GetRoot().Show(true);
            }
            m_ItemCards.Insert(card);
        }

        if (search_keyword != string.Empty)
            FilterItemList();
    }

    void OnPlayerSlotEventClick(PlayerSlotNavigationButtonComponent buttonViewController, int command = ETraderXClickEvents.LCLICK)
    {
        if (!buttonViewController || !buttonViewController.navBtnId)
            return;

        if (command == ETraderXClickEvents.LCLICK)
        {
            int slotId = EPlayerSlotNavigationButton.GetInventoryIdFromSlotId(buttonViewController.navBtnId);

            if (slotId == EPlayerSlotNavigationButton.ALL)
            {
                GetAllPlayerItems();
            }
            else if (buttonViewController.navBtnId == EPlayerSlotNavigationButton.VEHICLES)
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
        }

        if (command == ETraderXClickEvents.RCLICK)
        {
            PlayerSlotNavigationButtonComponent selectedNavBtn = PlayerSlotNavigationComponent.GetNavigationInstance().GetSelectedNavBtn();
            if (!selectedNavBtn)
                return;

            if (selectedNavBtn.navBtnId == buttonViewController.navBtnId)
            {
                ClearItemCards();
                return;
            }

            if (selectedNavBtn.navBtnId == EPlayerSlotNavigationButton.ALL)
            {
                GetAllPlayerItems();
                FillItemCardList();
            }
        }
    }

    void OnItemCardSelected(ItemCardComponent itemSelected, int command = ETraderXClickEvents.LCLICK)
    {
        if (TraderXProductStockRepository.IsStockReached(itemSelected.GetItem().productId, itemSelected.GetItem().maxStock))
            return;

        switch (command)
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
        }
    }

    void OnItemCardLClick(ItemCardComponent itemSelected)
    {
        int volume;
        itemSelected.GetItemVolume(volume);
        total_quantity = "Total Quantity: " + volume.ToString();
        minimal_quantity = "Minimal Needed: " + TraderXTradeQuantity.GetItemSellQuantity(itemSelected.item.className, itemSelected.item.tradeQuantity).ToString();
        if (totalQuantityText) totalQuantityText.SetText(total_quantity);
        if (minimalQuantityText) minimalQuantityText.SetText(minimal_quantity);
    }

    void OnItemCardRClick(ItemCardComponent itemSelected)
    {
        if (!selected_item_sell)
            return;

        foreach (ItemCardComponent itemCard : m_ItemCards)
        {
            if (itemCard.preview.GetType() == itemSelected.preview.GetType())
                itemCard.SetItemSelected(true);
        }
    }

    void OnItemCardDoubleClick(ItemCardComponent itemDoubleClicked)
    {
        if (!manual_sell)
            return;

        if (TraderXTradingService.GetInstance().IsTransactionPending())
            return;

        int npcId = TraderXTradingService.GetInstance().GetNpcId();
        int sellPrice = itemDoubleClicked.GetPrice();

        TraderXTransaction sellTransaction = TraderXTransaction.CreateSellTransaction(itemDoubleClicked.item, 1, sellPrice, npcId);

        TraderXTransactionCollection transactionCollection = new TraderXTransactionCollection();
        transactionCollection.AddTransaction(sellTransaction);

        TraderXTradingService.GetInstance().LockTransaction();
        GetDayZularRPCManager().SendRPC("TraderX", "GetTransactionsRequest", new Param2<TraderXTransactionCollection, int>(transactionCollection, npcId));
    }

    void OnItemCardShiftLClick(ItemCardComponent itemShiftLClicked)
    {
        if (manual_sell)
            return;

        array<EntityAI> entityItems = new array<EntityAI>();
        TraderXInventoryManager.GetEntitiesChildren(itemShiftLClicked.preview, entityItems);

        itemShiftLClicked.SetItemSelected(true);

        foreach (ItemCardComponent itemCard : m_ItemCards)
        {
            if (entityItems.Find(itemCard.preview) != -1 && itemCard.selectable)
                itemCard.SetItemSelected(true);
        }
    }

    void OnItemCardShiftRClick(ItemCardComponent itemShiftRClicked)
    {
        if (manual_sell)
            return;
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (w == entireContainerBtn)
        {
            SetSellMode(ESellMode.ENTIRE_CONTAINER);
            return true;
        }
        if (w == selectedItemBtn)
        {
            SetSellMode(ESellMode.SELECTED_ITEM);
            return true;
        }
        if (w == manualBtn)
        {
            SetSellMode(ESellMode.MANUAL);
            return true;
        }
        return false;
    }

    override bool OnChange(Widget w, int x, int y, bool finished)
    {
        if (w == searchBox)
        {
            search_keyword = searchBox.GetText();
            GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(DebouncedSearch);
            GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(DebouncedSearch, 300, false);
            return true;
        }
        if (w == quantitySlider)
        {
            slider_quantity = SliderWidget.Cast(w).GetCurrent();
            TraderXTradingService.GetInstance().SetMaxQuantity(slider_quantity == 1.0);
            Event_OnSliderQuantityChangedCallBack.Invoke(slider_quantity == 1.0);
            return true;
        }
        return false;
    }

    void DebouncedSearch()
    {
        FilterItemList();
    }

    void FilterItemList()
    {
        foreach (ItemCardComponent card : m_ItemCards)
        {
            TraderXProduct item = card.item;
            if (item.className.Contains(search_keyword) || item.GetDisplayName().Contains(search_keyword) || search_keyword == string.Empty)
                card.GetRoot().Show(true);
            else
                card.GetRoot().Show(false);
        }
    }

    void OnTraderXResponseReceived(int responseReceived, TraderXTransactionResultCollection transactionResultCollection = null)
    {
        if (responseReceived == ETraderXResponse.ALL_STOCK_RECEIVED)
        {
            foreach (ItemCardComponent card : m_ItemCards)
            {
                card.UpdateStock();
            }
        }

        if (responseReceived == ETraderXResponse.TRANSACTIONS)
        {
            Refresh();
            TraderXInventoryManager.PlayMenuSound(ETraderXSounds.COINS);
        }
    }

    void Refresh()
    {
        ClearItemCards();
        CheckoutComponent.GetInstance().UpdateCheckoutView();
        InitPlayerItems();
        OnPlayerSlotEventClick(PlayerSlotNavigationComponent.GetNavigationInstance().GetSelectedNavBtn());
    }
}
