class CheckoutCardViewController: ViewController
{
    ref TraderXProduct item;
    string product_name_text;
    string product_quantity_text;
    string product_price_text;

    private int price;
    private int quantity;
    private int tradeMode;

    Widget btnPlus, btnMinus;

    static ref ScriptInvoker Event_OnPriceChanged = new ScriptInvoker();

    void Setup(TraderXProduct item, int quantity = 1)
    {
        this.item = item;
        this.quantity = quantity;
        this.tradeMode = TraderXTradingService.GetInstance().GetTradeMode();
        this.price = GetBasePrice();
        
        // Subscribe to multiplier change events
        TraderXProduct.Event_OnMultiplierChanged.Insert(OnProductMultiplierChanged);
        
        // Subscribe to checkout pricing changes for dynamic price updates
        TraderXProduct.Event_OnCheckoutPricingChanged.Insert(OnCheckoutPricingChanged);

        if(item.GetPlayerItem())
        {
            product_name_text = item.GetDisplayName() + " [" + item.defaultPreset.presetName + "]";
        }
        else
        {
            // In customize page, don't show preset name in checkout cards - show individual item names
            // In purchase page, show preset name for main product with preset
            if(TraderXUINavigationService.GetInstance().GetNavigationId() == ENavigationIds.CUSTOMIZE)
            {
                product_name_text = item.GetDisplayName();
            }
            else if(item.defaultPreset)
            {
                product_name_text = item.GetDisplayName() + " [" + item.defaultPreset.presetName + "]";
            }
            else
            {
                product_name_text = item.GetDisplayName();
            }
        }

        if(tradeMode == ETraderXTradeMode.SELL){
            btnPlus.Show(false);
            btnMinus.Show(false);
        }
        
        RefreshQuantityAndPrice();

        NotifyPropertiesChanged({"product_name_text"});
        OnSliderQuantityChanged(TraderXTradingService.GetInstance().IsMaxQuantity());         
    }

    int GetTradeMode()
    {
        return tradeMode;
    }

    int GetBasePrice()
    {
        if(tradeMode == ETraderXTradeMode.BUY){
            // In customize page, show individual item prices (base price for main product, individual prices for attachments)
            // In purchase page, show preset total price for main product with preset
            if(TraderXUINavigationService.GetInstance().GetNavigationId() == ENavigationIds.CUSTOMIZE)
            {
                // Always show individual item price in customize page - use dynamic pricing
                return TraderXPricingService.GetInstance().GetPricePreview(item.GetProductId(), true, 1, TraderXItemState.PRISTINE);
            }
            else if(item.defaultPreset)
            {
                // In purchase page, show preset total price for main product
                int presetPrice = TraderXPresetsService.GetInstance().CalculateTotalPricePreset(item.defaultPreset);
                return presetPrice;
            }
            else
            {
                // Fallback to individual attachment pricing - use dynamic pricing
                return TraderXPricingService.GetInstance().GetPricePreview(item.GetProductId(), true, 1, TraderXItemState.PRISTINE);
            }
        }

        if(tradeMode == ETraderXTradeMode.SELL){
            // Use dynamic sell pricing based on current stock and coefficient
            return TraderXPricingService.GetInstance().GetPricePreview(item.GetProductId(), false, 1, item.GetPlayerItem().healthLevel);
        }
        
        return 0;
    }

    void SetQuantity(int quantity)
    {
        this.quantity = quantity;
        product_quantity_text = Ternary<string>.If(tradeMode == ETraderXTradeMode.BUY, "+", Ternary<string>.If(tradeMode == ETraderXTradeMode.SELL, "-", "")) + quantity.ToString();
        NotifyPropertyChanged("product_quantity_text");
    }

    void SetPrice(int newPrice)
    {
        this.price = newPrice;
        if(tradeMode == ETraderXTradeMode.SELL){
            if(price < 0)
                price = 0;

            if(price == 0){
                product_price_text = "0";
            } else {
                product_price_text = "-" + TraderXQuantityManager.GetFormattedMoneyAmount(price);
            }
        } else {
            product_price_text = TraderXQuantityManager.GetFormattedMoneyAmount(price);
        }
        NotifyPropertyChanged("product_price_text");
    }

    int GetPrice()
    {
        return price;
    }

    int GetUnitPrice()
    {
        return GetBasePrice();
    }

    TraderXProduct GetItem()
    {
        return item;
    }
    
    void RefreshQuantityAndPrice()
    {
        SetQuantity(item.GetMultiplier());
        
        // Get the correct price based on multiplier - don't multiply again
        int totalPrice = 0;
        if(tradeMode == ETraderXTradeMode.BUY){
            if(TraderXUINavigationService.GetInstance().GetNavigationId() == ENavigationIds.CUSTOMIZE)
            {
                // Individual item price with multiplier
                totalPrice = TraderXPricingService.GetInstance().GetPricePreview(item.GetProductId(), true, item.GetMultiplier(), TraderXItemState.PRISTINE);
            }
            else if(item.defaultPreset)
            {
                // Preset price with progressive multiplier pricing
                totalPrice = TraderXPresetsService.GetInstance().CalculateProgressivePresetPrice(item.defaultPreset, item.GetMultiplier());
            }
            else
            {
                // Individual item price with multiplier
                totalPrice = TraderXPricingService.GetInstance().GetPricePreview(item.GetProductId(), true, item.GetMultiplier(), TraderXItemState.PRISTINE);
            }
        }
        else if(tradeMode == ETraderXTradeMode.SELL){
            // Use checkout pricing service for dynamic sell prices that consider other items in checkout
            TraderXPriceCalculation priceCalc;
            string itemKey = item.GetPlayerItem().GetPlayerItemId();
            
            GetTraderXLogger().LogDebug(string.Format("[CHECKOUT_CARD] Calculating sell price for item: %1 (key: %2)", item.GetProductId(), itemKey));
            
            if(item.defaultPreset) {
                // Use preset sell pricing with checkout consideration for this specific item
                priceCalc = TraderXCheckoutPricingService.GetInstance().CalculatePresetSellPriceWithCheckoutForItem(item.defaultPreset, itemKey, item.GetMultiplier(), item.GetPlayerItem().healthLevel);
            } else {
                // Regular product sell pricing with checkout consideration for this specific item
                priceCalc = TraderXCheckoutPricingService.GetInstance().CalculateSellPriceWithCheckoutForItem(TraderXProductRepository.GetItemById(item.GetProductId()), itemKey, item.GetMultiplier(), item.GetPlayerItem().healthLevel);
            }
            totalPrice = priceCalc.GetCalculatedPrice();
            
            GetTraderXLogger().LogDebug(string.Format("[CHECKOUT_CARD] Calculated sell price: %1 for item: %2", totalPrice, item.GetProductId()));
        }
        
        SetPrice(totalPrice);
        NotifyPropertiesChanged({"product_quantity_text", "product_price_text"});
        Event_OnPriceChanged.Invoke();
    }
    
    void OnProductMultiplierChanged(TraderXProduct product)
    {
        RefreshQuantityAndPrice();
    }
    
    void OnCheckoutPricingChanged()
    {
        // Only refresh pricing for sell items when checkout changes
        if (tradeMode == ETraderXTradeMode.SELL) {
            GetTraderXLogger().LogDebug(string.Format("[CHECKOUT_CARD] Refreshing pricing for sell item: %1", item.GetProductId()));
            RefreshQuantityAndPrice();
        }
    }

    void ResetPriceQuantityToNominal()
    {
        SetQuantity(1);
        SetPrice(GetBasePrice());
        item.SetMultiplier(1);
    }

    void OnSliderQuantityChanged(bool isMaxQuantity)
    {
        ResetPriceQuantityToNominal();
        if(!isMaxQuantity){
            return;
        }

        int sellMode = TraderXTradeQuantity.GetSellQuantityMode(item.tradeQuantity);
        if( sellMode != TraderXTradeQuantity.SELL_STATIC && sellMode != TraderXTradeQuantity.SELL_COEFFICIENT)
            return;

        ItemBase itemB = ItemBase.Cast(GetGame().GetObjectByNetworkId(item.playerItem.networkIdLow, item.playerItem.networkIdHigh));
        if(!itemB)
            return;

        int itemQuantity = TraderXQuantityManager.GetItemAmount(itemB);
        int tradeQty = TraderXTradeQuantity.GetItemSellQuantity(itemB.GetType(), item.tradeQuantity);
        if(tradeQty == 0)
            return;

        int newTradeQty = tradeQty;
        while((itemQuantity - tradeQty) > 0){
            itemQuantity -= tradeQty;
            newTradeQty += tradeQty;
        }

        item.SetMultiplier(newTradeQty/tradeQty);

        RefreshQuantityAndPrice(); // Use proper dynamic pricing instead of simple multiplication
    }

    bool OnPlusExecute(ButtonCommandArgs args)
    {
        if(tradeMode == ETraderXTradeMode.BUY){
            item.SetMultiplier(item.GetMultiplier() + 1);
            RefreshQuantityAndPrice(); // Use proper dynamic pricing instead of simple multiplication
            return true;
        }
        return false;
    }

    bool OnMinusExecute(ButtonCommandArgs args)
    {
        if(tradeMode == ETraderXTradeMode.BUY){
            if(item.GetMultiplier() == 1){
                return false;
            }
            
            item.SetMultiplier(item.GetMultiplier() - 1);
            RefreshQuantityAndPrice(); // Use proper dynamic pricing instead of simple multiplication
            return true;
        }
        return false;
    }
}