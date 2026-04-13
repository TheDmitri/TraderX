class CheckoutCardComponent extends DZlrComponentBase
{
    private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("traderx-checkout-card", CheckoutCardComponent);

    ref TraderXProduct item;
    string product_name_text;
    string product_quantity_text;
    string product_price_text;

    private int price;
    private int quantity;
    private int tradeMode;

    // Widgets
    Widget btnPlus, btnMinus;
    TextWidget productPriceText, productQtyText, productNameText;

    static ref ScriptInvoker Event_OnPriceChanged = new ScriptInvoker();

    void CheckoutCardComponent(TraderXProduct item)
    {
        this.item = item;
    }

    override protected void RegisterMetadata()
    {
        m_Metadata = new ComponentMetadata();
        m_Metadata.selector = "traderx-checkout-card";
        m_Metadata.templatePath = "TraderX/datasets/gui/TraderXPrefab/Checkout/CheckoutCardView.layout";
    }

    override void OnInit()
    {
        TraderXProduct.Event_OnMultiplierChanged.Insert(OnProductMultiplierChanged);
        TraderXProduct.Event_OnCheckoutPricingChanged.Insert(OnCheckoutPricingChanged);
        Setup(item);
    }

    override void OnDestroy()
    {
        TraderXProduct.Event_OnMultiplierChanged.Remove(OnProductMultiplierChanged);
        TraderXProduct.Event_OnCheckoutPricingChanged.Remove(OnCheckoutPricingChanged);
    }

    int GetTradeMode()
    {
        return tradeMode;
    }

    TraderXProduct GetItem()
    {
        return item;
    }

    int GetPrice()
    {
        return price;
    }

    void Setup(TraderXProduct item, int quantity = 1)
    {
        this.item = item;
        this.quantity = quantity;
        this.tradeMode = TraderXTradingService.GetInstance().GetTradeMode();
        this.price = GetBasePrice();

        if (item.GetPlayerItem())
        {
            product_name_text = item.GetDisplayName() + " [" + item.defaultPreset.presetName + "]";
        }
        else
        {
            if (TraderXUINavigationService.GetInstance().GetNavigationId() == ENavigationIds.CUSTOMIZE)
                product_name_text = item.GetDisplayName();
            else if (item.defaultPreset)
                product_name_text = item.GetDisplayName() + " [" + item.defaultPreset.presetName + "]";
            else
                product_name_text = item.GetDisplayName();
        }

        if (tradeMode == ETraderXTradeMode.SELL)
        {
            if (btnPlus) btnPlus.Show(false);
            if (btnMinus) btnMinus.Show(false);
        }

        RefreshQuantityAndPrice();

        if (productNameText) productNameText.SetText(product_name_text);
        OnSliderQuantityChanged(TraderXTradingService.GetInstance().IsMaxQuantity());
    }

    int GetBasePrice()
    {
        if (tradeMode == ETraderXTradeMode.BUY)
        {
            if (TraderXUINavigationService.GetInstance().GetNavigationId() == ENavigationIds.CUSTOMIZE)
                return TraderXPricingService.GetInstance().GetPricePreview(item.GetProductId(), true, 1, TraderXItemState.PRISTINE);
            else if (item.defaultPreset)
                return TraderXPresetsService.GetInstance().CalculateTotalPricePreset(item.defaultPreset);
            else
                return TraderXPricingService.GetInstance().GetPricePreview(item.GetProductId(), true, 1, TraderXItemState.PRISTINE);
        }

        if (tradeMode == ETraderXTradeMode.SELL)
            return TraderXPricingService.GetInstance().GetPricePreview(item.GetProductId(), false, 1, item.GetPlayerItem().healthLevel);

        return 0;
    }

    void SetQuantity(int qty)
    {
        this.quantity = qty;
        product_quantity_text = TernaryString(tradeMode == ETraderXTradeMode.BUY, "+", TernaryString(tradeMode == ETraderXTradeMode.SELL, "-", "")) + qty.ToString();
        if (productQtyText) productQtyText.SetText(product_quantity_text);
    }

    void SetPrice(int newPrice)
    {
        this.price = newPrice;
        if (tradeMode == ETraderXTradeMode.SELL)
        {
            if (price < 0) price = 0;
            if (price == 0)
                product_price_text = "0";
            else
                product_price_text = "-" + TraderXQuantityManager.GetFormattedMoneyAmount(price);
        }
        else
        {
            product_price_text = TraderXQuantityManager.GetFormattedMoneyAmount(price);
        }
        if (productPriceText) productPriceText.SetText(product_price_text);
    }

    void RefreshQuantityAndPrice()
    {
        SetQuantity(item.GetMultiplier());

        int totalPrice = 0;
        if (tradeMode == ETraderXTradeMode.BUY)
        {
            if (TraderXUINavigationService.GetInstance().GetNavigationId() == ENavigationIds.CUSTOMIZE)
                totalPrice = TraderXPricingService.GetInstance().GetPricePreview(item.GetProductId(), true, item.GetMultiplier(), TraderXItemState.PRISTINE);
            else if (item.defaultPreset)
                totalPrice = TraderXPresetsService.GetInstance().CalculateProgressivePresetPrice(item.defaultPreset, item.GetMultiplier());
            else
                totalPrice = TraderXPricingService.GetInstance().GetPricePreview(item.GetProductId(), true, item.GetMultiplier(), TraderXItemState.PRISTINE);
        }
        else if (tradeMode == ETraderXTradeMode.SELL)
        {
            TraderXPriceCalculation priceCalc;
            string itemKey = item.GetPlayerItem().GetPlayerItemId();

            if (item.defaultPreset)
                priceCalc = TraderXCheckoutPricingService.GetInstance().CalculatePresetSellPriceWithCheckoutForItem(item.defaultPreset, itemKey, item.GetMultiplier(), item.GetPlayerItem().healthLevel);
            else
                priceCalc = TraderXCheckoutPricingService.GetInstance().CalculateSellPriceWithCheckoutForItem(TraderXProductRepository.GetItemById(item.GetProductId()), itemKey, item.GetMultiplier(), item.GetPlayerItem().healthLevel);
            totalPrice = priceCalc.GetCalculatedPrice();
        }

        SetPrice(totalPrice);
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
        item.SetMultiplier(1);
    }

    void OnSliderQuantityChanged(bool isMaxQuantity)
    {
        ResetPriceQuantityToNominal();
        if (!isMaxQuantity)
            return;

        int sellMode = TraderXTradeQuantity.GetSellQuantityMode(item.tradeQuantity);
        if (sellMode != TraderXTradeQuantity.SELL_STATIC && sellMode != TraderXTradeQuantity.SELL_COEFFICIENT)
            return;

        ItemBase itemB = ItemBase.Cast(GetGame().GetObjectByNetworkId(item.playerItem.networkIdLow, item.playerItem.networkIdHigh));
        if (!itemB)
            return;

        int itemQuantity = TraderXQuantityManager.GetItemAmount(itemB);
        int tradeQty = TraderXTradeQuantity.GetItemSellQuantity(itemB.GetType(), item.tradeQuantity);
        if (tradeQty == 0)
            return;

        int newTradeQty = tradeQty;
        while ((itemQuantity - tradeQty) > 0)
        {
            itemQuantity -= tradeQty;
            newTradeQty += tradeQty;
        }

        item.SetMultiplier(newTradeQty / tradeQty);
        RefreshQuantityAndPrice();
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (w == btnPlus)
        {
            if (tradeMode == ETraderXTradeMode.BUY)
            {
                item.SetMultiplier(item.GetMultiplier() + 1);
                RefreshQuantityAndPrice();
                return true;
            }
        }
        if (w == btnMinus)
        {
            if (tradeMode == ETraderXTradeMode.BUY)
            {
                if (item.GetMultiplier() == 1)
                    return false;
                item.SetMultiplier(item.GetMultiplier() - 1);
                RefreshQuantityAndPrice();
                return true;
            }
        }
        return false;
    }
}
