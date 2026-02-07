class CheckoutViewController: ViewController
{
    ref ObservableCollection<ref CheckoutCardView> checkout_card_list = new ObservableCollection<ref CheckoutCardView>(this);

    static ref ScriptInvoker Event_OnTotalPriceChange = new ScriptInvoker();

    int totalPrice;

    string total_amount;
    string bank_amount;
    string money_amount;

    private static ref CheckoutViewController m_Instance;

    static CheckoutViewController GetInstance()
    {
        return m_Instance;
    }
    
    array<ref CheckoutCardView> GetCheckoutItems()
    {
        return checkout_card_list.GetArray();
    }

    void CheckoutViewController()
    {
        m_Instance = this;
        TraderXSelectionService.Event_OnItemSelectionChanged.Insert(OnItemSelectionChanged);
        SellPageViewController.Event_OnSliderQuantityChangedCallBack.Insert(OnSliderQuantityChanged);
        TraderXTradingService.Event_OnTraderXResponseReceived.Insert(OnTraderXResponseReceived);
        CheckoutCardViewController.Event_OnPriceChanged.Insert(OnPriceChanged);
    }

    override void OnWidgetScriptInit(Widget w)
	{
		super.OnWidgetScriptInit(w);
        UpdateBankMoney();
        UpdatePlayerMoney();
    }

    void AddCheckoutItem(TraderXProduct item)
    {
        CheckoutCardView existingCard = GetCheckoutCardForItem(item);
        if (existingCard)
        {
            existingCard.GetTemplateController().RefreshQuantityAndPrice();
        }
        else
        {
            // Add item to checkout pricing service BEFORE creating the card
            if (item.GetPlayerItem()) { // Sell mode
                string uniqueKey = item.GetPlayerItem().GetPlayerItemId();
                GetTraderXLogger().LogDebug(string.Format("[CHECKOUT] Adding sell item to checkout: %1 (key: %2)", item.GetProductId(), uniqueKey));
                TraderXCheckoutPricingService.GetInstance().AddItemToCheckout(item.GetProductId(), item.GetMultiplier(), uniqueKey);
            }
            
            // Create the checkout card after adding to pricing service
            checkout_card_list.Insert(new CheckoutCardView(item));
        }
    }

    CheckoutCardView GetCheckoutCardForItem(TraderXProduct item)
    {
        foreach(CheckoutCardView checkoutCardView: checkout_card_list.GetArray()){
            if(!checkoutCardView)
                continue;

            TraderXProduct checkoutItem = checkoutCardView.GetTemplateController().GetItem();
            
            // For sell items, use unique playerItemId; for buy items, use productId
            if (item.GetPlayerItem() && checkoutItem.GetPlayerItem()) {
                if (item.GetPlayerItem().GetPlayerItemId() == checkoutItem.GetPlayerItem().GetPlayerItemId()) {
                    return checkoutCardView;
                }
            } else if (!item.GetPlayerItem() && !checkoutItem.GetPlayerItem()) {
                if (item.productId == checkoutItem.productId) {
                    return checkoutCardView;
                }
            }
        }
        
        return null;
    }

    void RemoveCheckoutItem(TraderXProduct item)
    {
        for(int i = checkout_card_list.GetArray().Count() - 1; i >= 0; i--)
        {
            if(item.GetPlayerItem()) {
                // Sell mode - compare by unique playerItemId
                if(item.GetPlayerItem().GetPlayerItemId() == checkout_card_list.Get(i).GetTemplateController().GetItem().GetPlayerItem().GetPlayerItemId())
                {
                    // Remove from checkout pricing service
                    TraderXCheckoutPricingService.GetInstance().RemoveItemFromCheckout(item.GetProductId(), item.GetMultiplier());
                    checkout_card_list.Remove(i);
                    return;
                }
            } else {
                // Buy mode - compare by productId
                if(item.productId == checkout_card_list.Get(i).GetTemplateController().GetItem().productId)
                {
                    checkout_card_list.Remove(i);
                    return;
                }
            }
        }
    }

    void RemoveCheckoutItemFromId(UUID productId)
    {
        for(int i = checkout_card_list.GetArray().Count() - 1; i >= 0; i--)
        {
            if(productId == checkout_card_list.Get(i).GetTemplateController().GetItem().productId)
            {
                checkout_card_list.Remove(i);
                return;
            }
        }
    }

    void OnItemSelectionChanged(TraderXProduct item)
    {
        if(item.isSelected){
            // Check if item already exists in checkout and refresh it
            CheckoutCardView existingCard = GetCheckoutCardForItem(item);
            if (existingCard)
            {
                existingCard.GetTemplateController().RefreshQuantityAndPrice();
            }
            else
            {
                AddCheckoutItem(item);
            }
        }
        else{ 
            RemoveCheckoutItem(item);
        }
        UpdateTotalPrice();
        UpdatePlayerMoney();
    }

    void UpdatePlayerMoney()
    {
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if(!player)
            return;
    
        int playerMoney = TraderXTradingService.GetInstance().GetPlayerMoneyAmount(player);
        money_amount = TraderXQuantityManager.GetFormattedMoneyAmount(playerMoney);
        NotifyPropertyChanged("money_amount");
    }

    void UpdateCheckoutView()
    {
        checkout_card_list.Clear();

        TraderXSelectionService selectionService = TraderXSelectionService.GetInstance();
        array<ref TraderXProduct> selectedItems = selectionService.GetSelectedItems();
        
        foreach (TraderXProduct item : selectedItems)
        {
            AddCheckoutItem(item);
        }

        UpdateTotalPrice();
        UpdatePlayerMoney();
    }

    void OnSliderQuantityChanged(bool isMaxQuantity)
    {
        foreach(CheckoutCardView checkoutCardView: checkout_card_list.GetArray()){
			if(!checkoutCardView)
				continue;

            checkoutCardView.GetTemplateController().OnSliderQuantityChanged(isMaxQuantity);
		}
        UpdateTotalPrice();
    }

    void UpdateBankMoney()
    {
        //TODO: implement this function once the banking mod is done
    }

    void OnPriceChanged()
    {
        UpdateTotalPrice();
    }

    int GetTotalPrice()
    {
        totalPrice = 0;
        foreach(CheckoutCardView checkoutCardView: checkout_card_list.GetArray()){
			if(!checkoutCardView)
				continue;

			totalPrice += checkoutCardView.GetTemplateController().GetPrice();
		}

        return totalPrice;
    }

    void UpdateTotalPrice()
    {
        totalPrice = GetTotalPrice();

        total_amount = TraderXQuantityManager.GetFormattedMoneyAmount(totalPrice);
        NotifyPropertyChanged("total_amount");
    }

    void OnTraderXResponseReceived(int responseReceived, TraderXTransactionResultCollection transactionResultCollection = null)
    {
        if (responseReceived != ETraderXResponse.TRANSACTIONS || !transactionResultCollection)
            return;

        array<UUID> itemsToDeselect = new array<UUID>();

        for(int i = 0; i < transactionResultCollection.GetTransactionResults().Count(); i++)
        {
            TraderXTransactionResult transactionResult = transactionResultCollection.GetTransactionResults()[i];
            CheckoutCardViewController checkoutCardController = GetCheckoutCardViewControllerFromId(transactionResult.GetProductId());
            if(!checkoutCardController)
                continue;

            if(transactionResult.IsSuccess()){
                itemsToDeselect.Insert(transactionResult.GetProductId());
            }
        }

        if (itemsToDeselect.Count() > 0){
            // In customize view, keep all items selected after successful purchase
            if(TraderXUINavigationService.GetInstance().GetNavigationId() == ENavigationIds.CUSTOMIZE) {
                // Don't deselect any items in customize view - keep everything selected for continued customization
                // This allows users to purchase multiple presets without losing their selection
            } else {
                // In other views (purchase page), deselect all items as usual
                TraderXSelectionService.GetInstance().DeselectItems(itemsToDeselect);
            }
            TraderXInventoryManager.PlayMenuSound(ETraderXSounds.COINS);
        }
                
        UpdateCheckoutView();
    }

    CheckoutCardViewController GetCheckoutCardViewControllerFromId(UUID productId)
    {
        foreach(CheckoutCardView checkoutCardView: checkout_card_list.GetArray()){	
            if(!checkoutCardView)
                continue;

            if(checkoutCardView.GetTemplateController().GetItem().productId == productId)
                return checkoutCardView.GetTemplateController();
        }

        return null;
    }

    void HandleCustomizeCheckout()
    {
        if(CustomizePageViewController.GetInstance()){
            TraderXProduct item = CustomizePageViewController.GetInstance().GetItem();
            array<string> attachments = new array<string>();

            foreach(CheckoutCardView checkoutCardView: checkout_card_list.GetArray()){
                if(!checkoutCardView)
                    continue;
    
                TraderXProduct checkoutItem = checkoutCardView.GetTemplateController().GetItem();
                if(checkoutItem.productId == item.productId)
                    continue;

                for(int i = 0; i < checkoutCardView.GetTemplateController().GetItem().GetMultiplier(); i++){
                    attachments.Insert(checkoutItem.productId);
                }
            }

            TraderXPreset preset = TraderXPreset.CreateTraderXPreset("Customize", item.productId, attachments);

            item.defaultPreset = preset;

            TraderXTransactionCollection transactions = new TraderXTransactionCollection();
            int npcId = TraderXTradingService.GetInstance().GetNpcId();
            transactions.AddTransaction(TraderXTransaction.CreateBuyTransaction(item, 1, GetTotalPrice(), npcId, item.defaultPreset));
            GetRPCManager().SendRPC("TraderX", "GetTransactionsRequest", new Param2<TraderXTransactionCollection, int>(transactions, npcId));
        }
    }

    bool OnClearCheckoutExecute(ButtonCommandArgs args)
    {
        TraderXInventoryManager.PlayMenuSound(ETraderXSounds.SELECT);
        if(checkout_card_list.GetArray().Count() == 0)
            return false;

        TraderXSelectionService.GetInstance().DeSelectAllItems();
        checkout_card_list.Clear();
        UpdateTotalPrice();
        UpdatePlayerMoney();
        return true;
    }

    bool OnCheckoutExecute(ButtonCommandArgs args)
    {
        TraderXInventoryManager.PlayMenuSound(ETraderXSounds.CONFIRM);
        if(checkout_card_list.GetArray().Count() == 0){
            return false;
        }

        if(TraderXTradingService.GetInstance().IsTransactionPending())
            return false;

        if(TraderXUINavigationService.GetInstance().GetNavigationId() == ENavigationIds.CUSTOMIZE){
            TraderXTradingService.GetInstance().LockTransaction();
            HandleCustomizeCheckout();
            return true;
        }
        
        TraderXTransactionCollection transactions = new TraderXTransactionCollection();
        int npcId = TraderXTradingService.GetInstance().GetNpcId();
        foreach(CheckoutCardView checkoutCardView: checkout_card_list.GetArray()){
			if(!checkoutCardView)
				continue;

            TraderXProduct item = checkoutCardView.GetTemplateController().GetItem();
            
            if(checkoutCardView.GetTemplateController().GetTradeMode() == ETraderXTradeMode.BUY)
            {
                int averageUnitPrice;
                // Check if item has a default preset applied
                if(item.defaultPreset)
                {
                    // For preset buy transactions, calculate average unit price but validate against progressive total on server
                    averageUnitPrice = checkoutCardView.GetTemplateController().GetPrice() / checkoutCardView.GetTemplateController().GetItem().GetMultiplier();
                    transactions.AddTransaction(TraderXTransaction.CreateBuyTransaction(item, checkoutCardView.GetTemplateController().GetItem().GetMultiplier(), averageUnitPrice, npcId, item.defaultPreset));
                }
                else
                {
                    // For regular buy transactions, use total price divided by multiplier to get average unit price
                    averageUnitPrice = checkoutCardView.GetTemplateController().GetPrice() / checkoutCardView.GetTemplateController().GetItem().GetMultiplier();
                    transactions.AddTransaction(TraderXTransaction.CreateBuyTransaction(item, checkoutCardView.GetTemplateController().GetItem().GetMultiplier(), averageUnitPrice, npcId));
                }
            }
            else if(checkoutCardView.GetTemplateController().GetTradeMode() == ETraderXTradeMode.SELL)
            {
                // For sell transactions with multipliers, create individual transactions for each item with correct progressive pricing
                int multiplier = checkoutCardView.GetTemplateController().GetItem().GetMultiplier();
                if(multiplier > 1)
                {
                    // Create individual transactions for each item with progressive pricing
                    TraderXPriceCalculation priceCalc;
                    string itemKey = item.GetPlayerItem().GetPlayerItemId();
                    
                    for(int i = 0; i < multiplier; i++)
                    {
                        if(item.defaultPreset) {
                            // Calculate price for this specific unit considering previous units sold
                            priceCalc = TraderXCheckoutPricingService.GetInstance().CalculatePresetSellPriceWithCheckoutForItem(item.defaultPreset, itemKey, i + 1, item.GetPlayerItem().healthLevel);
                        } else {
                            // Calculate price for this specific unit considering previous units sold
                            priceCalc = TraderXCheckoutPricingService.GetInstance().CalculateSellPriceWithCheckoutForItem(TraderXProductRepository.GetItemById(item.GetProductId()), itemKey, i + 1, item.GetPlayerItem().healthLevel);
                        }
                        
                        // Get the price for just this unit (difference between cumulative prices)
                        int thisUnitPrice;
                        if(i == 0) {
                            thisUnitPrice = priceCalc.GetCalculatedPrice();
                        } else {
                            TraderXPriceCalculation prevPriceCalc;
                            if(item.defaultPreset) {
                                prevPriceCalc = TraderXCheckoutPricingService.GetInstance().CalculatePresetSellPriceWithCheckoutForItem(item.defaultPreset, itemKey, i, item.GetPlayerItem().healthLevel);
                            } else {
                                prevPriceCalc = TraderXCheckoutPricingService.GetInstance().CalculateSellPriceWithCheckoutForItem(TraderXProductRepository.GetItemById(item.GetProductId()), itemKey, i, item.GetPlayerItem().healthLevel);
                            }
                            thisUnitPrice = priceCalc.GetCalculatedPrice() - prevPriceCalc.GetCalculatedPrice();
                        }
                        
                        transactions.AddTransaction(TraderXTransaction.CreateSellTransaction(item, 1, thisUnitPrice, npcId));
                    }
                }
                else
                {
                    // Single item - use the calculated price
                    transactions.AddTransaction(TraderXTransaction.CreateSellTransaction(item, 1, checkoutCardView.GetTemplateController().GetPrice(), npcId));
                }
            }
		}

        TraderXTradingService.GetInstance().LockTransaction();
        GetRPCManager().SendRPC("TraderX", "GetTransactionsRequest", new Param2<TraderXTransactionCollection, int>(transactions, npcId));

        TraderXInventoryManager.PlayMenuSound(ETraderXSounds.CONFIRM);
        return true;
    }
}