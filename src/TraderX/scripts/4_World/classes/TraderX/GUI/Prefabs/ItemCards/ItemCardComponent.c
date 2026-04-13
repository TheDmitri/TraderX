class ItemCardComponent extends DZlrComponentBase
{
    private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("traderx-item-card", ItemCardComponent);

    ref TraderXProduct item;
    string itemName, itemPrice, itemStock;
    float itemVolume;
    EntityAI preview;
    int tradeMode;
    int categoryType;
    bool selectable, isFavable, isFav;

    // Widgets
    Widget itemCardOutline, itemCardContent, stockText;
    ImageWidget selectedIcon, unselectedIcon, favIcon, unfavIcon, healthImg;
    Widget volumeBar, favBtn;
    ButtonWidget itemCardMainBtn, volumeBtn;
    TextWidget itemNameText, itemPriceText, itemStockText;
    ItemPreviewWidget itemPreviewWidget;

    ref NavBtnTooltipComponent navBtnTooltipView;
    ref SellTooltipComponent sellTooltipView;
    ref BuyTooltipComponent buyTooltipView;
    ref CustomizeTooltipComponent customizeTooltipView;

    string quantityStr;

    static ref ScriptInvoker Event_OnItemCardClickEventCallBack = new ScriptInvoker();
    static ref ScriptInvoker Event_OnItemCardSelectCallBack = new ScriptInvoker();
    static ref ScriptInvoker Event_OnDestroyAllTooltips = new ScriptInvoker();

    override protected void RegisterMetadata()
    {
        m_Metadata = new ComponentMetadata();
        m_Metadata.selector = "traderx-item-card";
        m_Metadata.templatePath = "TraderX/datasets/gui/TraderXPrefab/ItemCard/ItemCardLargeView.layout";
    }

    void ItemCardComponent()
    {
        TraderXSelectionService.Event_OnItemSelectionChanged.Insert(OnItemSelectionChanged);
        TraderXTradingService.Event_OnTraderXResponseReceived.Insert(OnTraderXResponseReceived);
        TraderXProduct.Event_OnCheckoutPricingChanged.Insert(OnCheckoutPricingChanged);
        Event_OnDestroyAllTooltips.Insert(DestroyTooltips);
    }

    void ~ItemCardComponent()
    {
        TraderXSelectionService.Event_OnItemSelectionChanged.Remove(OnItemSelectionChanged);
        TraderXTradingService.Event_OnTraderXResponseReceived.Remove(OnTraderXResponseReceived);
        TraderXProduct.Event_OnCheckoutPricingChanged.Remove(OnCheckoutPricingChanged);
        Event_OnDestroyAllTooltips.Remove(DestroyTooltips);
        DestroyTooltips();
    }

    static void DestroyAllTooltips()
    {
        Event_OnDestroyAllTooltips.Invoke();
    }

    static ItemCardComponent CreateItemCard(TraderXProduct item, int size, int categoryType = ETraderXCategoryType.NONE, bool selectable = true, bool isFavable = true)
    {
        ItemCardComponent card = new ItemCardComponent();
        card.SetLayoutForSize(size);
        card.InitComponent(null);
        card.Setup(item, categoryType, selectable, isFavable, false);
        return card;
    }

    void SetLayoutForSize(int size)
    {
        switch (size)
        {
            case EItemCardSize.SMALL:
                m_Metadata.templatePath = "TraderX/datasets/gui/TraderXPrefab/ItemCard/ItemCardSmallView.layout";
                break;
            case EItemCardSize.MEDIUM:
                m_Metadata.templatePath = "TraderX/datasets/gui/TraderXPrefab/ItemCard/ItemCardMediumView.layout";
                break;
            case EItemCardSize.TOOLTIP:
                m_Metadata.templatePath = "TraderX/datasets/gui/TraderXPrefab/ItemCard/ItemCardMediumViewTooltip.layout";
                break;
            default:
                m_Metadata.templatePath = "TraderX/datasets/gui/TraderXPrefab/ItemCard/ItemCardLargeView.layout";
                break;
        }
    }

    void Setup(TraderXProduct item, int categoryType, bool selectable, bool isFavable, bool isFav)
    {
        this.item = item;
        this.tradeMode = TraderXTradingService.GetInstance().GetTradeMode();
        this.categoryType = categoryType;
        this.selectable = selectable;
        this.isFavable = isFavable;

        if (isFavable)
        {
            this.isFav = TraderXFavoritesService.GetInstance().IsFavorite(item);
            UpdateFav();
        }

        ShowName();
        UpdateStock();
        bool showBar = GetItemVolumePerCent(itemVolume);
        if (volumeBar) volumeBar.Show(showBar);

        CreatePreview();

        itemPrice = GetPriceFromItem();
        SetItemHealth();

        if (item.isSelected)
            Event_OnItemCardSelectCallBack.Invoke(this);

        UpdateWidgets();
        OnShow(true);
    }

    void UpdateWidgets()
    {
        if (itemNameText) itemNameText.SetText(itemName);
        if (itemPriceText) itemPriceText.SetText(itemPrice);
        if (itemPreviewWidget) itemPreviewWidget.SetItem(preview);
    }

    void ShowName()
    {
        itemName = item.GetDisplayName();
    }

    bool IsStockEmpty()
    {
        return TraderXProductStockRepository.GetStockValue(item.productId) == 0;
    }

    void UpdateStock()
    {
        if (item.IsStockUnlimited())
        {
            if (stockText) stockText.Show(false);
            return;
        }

        if (itemCardContent) itemCardContent.SetColor(TraderXViewStyles.ITEM_CARD_IDLE);
        selectable = true;

        if (tradeMode == ETraderXTradeMode.SELL)
        {
            if (TraderXProductStockRepository.IsStockReached(item.productId, item.maxStock))
            {
                if (stockText) stockText.SetColor(ARGB(255, 255, 0, 0));
                selectable = false;
            }
        }

        if (tradeMode == ETraderXTradeMode.BUY)
        {
            if (TraderXProductStockRepository.GetStockValue(item.productId) == 0)
            {
                if (stockText) stockText.SetColor(ARGB(255, 255, 0, 0));
                selectable = false;
            }
        }

        itemStock = TraderXProductStockRepository.GetStockDisplay(item.productId, item.maxStock);
        if (itemStockText) itemStockText.SetText(itemStock);
    }

    void SetItemHealth()
    {
        int a, r, g, b;
        if (tradeMode == ETraderXTradeMode.SELL)
        {
            ItemBase itemB = ItemBase.Cast(GetGame().GetObjectByNetworkId(item.playerItem.networkIdLow, item.playerItem.networkIdHigh));
            if (!itemB)
                return;

            float health = itemB.GetHealthLevel();
            TraderXCoreUtils.GetHealthFromLevel(health, a, r, g, b);
            if (healthImg) healthImg.SetColor(ARGB(a, r, g, b));
        }
    }

    void CreatePreview()
    {
        if (tradeMode == ETraderXTradeMode.SELL)
        {
            preview = GetGame().GetObjectByNetworkId(item.playerItem.networkIdLow, item.playerItem.networkIdHigh);
            if (!preview)
                preview = EntityAI.Cast(GetGame().CreateObjectEx(item.className, vector.Zero, ECE_LOCAL | ECE_NOLIFETIME));
        }
        else
        {
            preview = EntityAI.Cast(GetGame().CreateObjectEx(item.className, vector.Zero, ECE_LOCAL | ECE_NOLIFETIME));
            LookForDefaultPreset();
            CreateAttachments();
        }

        if (itemPreviewWidget) itemPreviewWidget.SetItem(preview);
    }

    void LookForDefaultPreset()
    {
        TraderXPreset defaultPreset = TraderXPresetsService.GetInstance().GetDefaultPreset(item.productId);
        if (!defaultPreset)
            defaultPreset = TraderXPresetsService.GetInstance().GetServerDefaultPreset(item.productId);

        if (!defaultPreset)
            return;

        item.defaultPreset = defaultPreset;
        item.selectedAttachments = defaultPreset.attachments;
    }

    void CreateAttachments()
    {
        if (!item || !item.selectedAttachments)
            return;

        foreach (string productId : item.selectedAttachments)
        {
            TraderXProduct attachment = TraderXProductRepository.GetItemById(productId);
            if (!attachment)
                continue;

            EntityAI entAttach = EntityAI.Cast(GetGame().CreateObjectEx(attachment.className, vector.Zero, ECE_LOCAL | ECE_NOLIFETIME));
            if (!entAttach)
                continue;

            TraderXAttachmentHandler.TryAttachItem(preview, entAttach);
        }
    }

    EntityAI GetPreview()
    {
        return preview;
    }

    string GetPriceFromItem()
    {
        if (tradeMode == ETraderXTradeMode.SELL)
        {
            if (item.GetPlayerItem())
            {
                int dynamicPrice = TraderXPricingService.GetInstance().GetPricePreview(item.GetProductId(), false, 1, item.GetPlayerItem().healthLevel);
                return dynamicPrice.ToString();
            }
            else
            {
                int catalogSellPrice = TraderXPricingService.GetInstance().GetPricePreview(item.GetProductId(), false, 1, TraderXItemState.PRISTINE);
                return catalogSellPrice.ToString();
            }
        }

        if (tradeMode == ETraderXTradeMode.BUY)
        {
            int basePrice = TraderXPricingService.GetInstance().GetPricePreview(item.GetProductId(), true, 1, TraderXItemState.PRISTINE);

            if (item.defaultPreset)
            {
                int presetPrice = TraderXPresetsService.GetInstance().CalculateTotalPricePreset(item.defaultPreset);
                basePrice = presetPrice;
            }
            else
            {
                basePrice += CalculateAttachmentsPrice();
            }

            return basePrice.ToString();
        }

        return string.Empty;
    }

    int CalculateAttachmentsPrice()
    {
        int price = 0;
        if (!item.selectedAttachments)
            return price;

        foreach (UUID productId : item.selectedAttachments)
        {
            TraderXProduct attachment = TraderXProductRepository.GetItemById(productId);
            if (!attachment)
                continue;
            price += TraderXPricingService.GetInstance().GetPricePreview(attachment.GetProductId(), true, 1, TraderXItemState.PRISTINE);
        }
        return price;
    }

    bool GetItemVolumePerCent(out float volume)
    {
        ItemBase itemB;
        if (tradeMode == ETraderXTradeMode.SELL)
        {
            itemB = ItemBase.Cast(GetGame().GetObjectByNetworkId(item.playerItem.networkIdLow, item.playerItem.networkIdHigh));
            if (!itemB || !itemB.HasQuantity())
                return false;
            volume = TraderXQuantityManager.GetItemVolumePerCent(itemB);
            return true;
        }

        if (tradeMode == ETraderXTradeMode.BUY)
        {
            if (!preview)
                return false;
            itemB = ItemBase.Cast(preview);
            if (!itemB || !itemB.HasQuantity())
                return false;

            int tradeQuantity = TraderXTradeQuantity.GetItemBuyQuantityClient(preview, item.tradeQuantity);
            int maxQuantity = TraderXQuantityManager.GetMaxItemQuantityClient(itemB);
            if (maxQuantity <= 0)
                return false;

            quantityStr = tradeQuantity.ToString() + "/" + maxQuantity;
            volume = Math.Round((tradeQuantity * 100) / maxQuantity);
            return true;
        }

        return false;
    }

    bool GetItemVolume(out float volume)
    {
        volume = GetItemVolumePerCent(volume) / 100;
        return true;
    }

    TraderXProduct GetItem()
    {
        return item;
    }

    int GetPrice()
    {
        return itemPrice.ToInt();
    }

    string GetItemDescription()
    {
        if (!preview)
            return string.Empty;

        InventoryItem iItem = InventoryItem.Cast(preview);
        if (iItem)
            return TraderXCoreUtils.TrimUnt(iItem.GetTooltip());

        return string.Empty;
    }

    void OnShow(bool show)
    {
        if (show)
        {
            if (favIcon) favIcon.Show(isFav && isFavable);
            if (unfavIcon) unfavIcon.Show(!isFav && isFavable);
            if (selectedIcon) selectedIcon.Show(item.isSelected && selectable);
            if (itemCardOutline) itemCardOutline.SetAlpha(TernaryFloat(item.isSelected, 1.0, 0.0));
            if (unselectedIcon) unselectedIcon.Show(!item.isSelected && selectable);
            if (healthImg) healthImg.Show(tradeMode == ETraderXTradeMode.SELL);
        }
        else
        {
            DestroyTooltips();
        }
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (w == favBtn)
        {
            OnItemFavExecute();
            return true;
        }

        if (w != itemCardMainBtn)
            return false;

        if (!selectable)
            return false;

        if (button == MouseState.LEFT)
        {
            if (KeyState(KeyCode.KC_LSHIFT) == 0)
                SetItemSelected(!item.isSelected);

            Event_OnItemCardClickEventCallBack.Invoke(this, TernaryInt(KeyState(KeyCode.KC_LSHIFT) != 0, ETraderXClickEvents.SHIFT_LCLICK, ETraderXClickEvents.LCLICK));
            return true;
        }

        if (button == MouseState.RIGHT)
        {
            Event_OnItemCardClickEventCallBack.Invoke(this, TernaryInt(KeyState(KeyCode.KC_LSHIFT) != 0, ETraderXClickEvents.SHIFT_RCLICK, ETraderXClickEvents.RCLICK));
            return true;
        }

        return false;
    }

    override bool OnDoubleClick(Widget w, int x, int y, int button)
    {
        if (!selectable)
            return false;

        if (button == MouseState.LEFT)
        {
            Event_OnItemCardClickEventCallBack.Invoke(this, ETraderXClickEvents.DOUBLE_CLICK);
            return true;
        }
        return false;
    }

    override bool OnMouseEnter(Widget w, int x, int y)
    {
        if (w && w.GetUserID() == EButtonIds.SELECTED && selectable)
        {
            if (!item.isSelected)
            {
                if (itemCardOutline) itemCardOutline.SetAlpha(0.5);
                if (selectable)
                {
                    if (unselectedIcon) unselectedIcon.Show(false);
                    if (selectedIcon)
                    {
                        selectedIcon.Show(true);
                        selectedIcon.SetAlpha(0.5);
                    }
                }
            }
            ShowTooltip();
        }

        if (w && w.GetUserID() == 102)
        {
            if (quantityStr == string.Empty)
                return false;
            navBtnTooltipView = new NavBtnTooltipComponent(quantityStr, 10.0, 10.0);
            navBtnTooltipView.InitComponent(null);
            return true;
        }
        return true;
    }

    void ShowTooltip()
    {
        if (!preview)
            return;

        if (tradeMode == ETraderXTradeMode.BUY && categoryType == ETraderXCategoryType.NONE)
        {
            buyTooltipView = new BuyTooltipComponent(GetItemDescription(), item.HasVariants(), item.HasAttachments(), -0.273438, -0.104167);
            buyTooltipView.InitComponent(null);
            if (buyTooltipView.GetRoot()) buyTooltipView.GetRoot().Show(true);
            return;
        }

        if (tradeMode == ETraderXTradeMode.BUY && categoryType != ETraderXCategoryType.NONE)
        {
            customizeTooltipView = new CustomizeTooltipComponent(GetItemDescription());
            customizeTooltipView.InitComponent(null);
            if (customizeTooltipView.GetRoot()) customizeTooltipView.GetRoot().Show(true);
            return;
        }

        if (tradeMode == ETraderXTradeMode.SELL)
        {
            sellTooltipView = new SellTooltipComponent(GetItemDescription(), TraderXInventoryManager.GetEntityHierarchy(preview), -0.273438, -0.104167);
            sellTooltipView.InitComponent(null);
            if (sellTooltipView.GetRoot()) sellTooltipView.GetRoot().Show(true);
            return;
        }
    }

    void DestroyTooltips()
    {
        if (buyTooltipView)
        {
            delete buyTooltipView;
            buyTooltipView = null;
        }
        if (sellTooltipView)
        {
            delete sellTooltipView;
            sellTooltipView = null;
        }
        if (customizeTooltipView)
        {
            delete customizeTooltipView;
            customizeTooltipView = null;
        }
    }

    override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
    {
        if (w && w.GetUserID() == EButtonIds.SELECTED && selectable)
        {
            if (!item.isSelected)
            {
                if (itemCardOutline) itemCardOutline.SetAlpha(0.0);
                if (selectable)
                {
                    if (unselectedIcon) unselectedIcon.Show(true);
                    if (selectedIcon)
                    {
                        selectedIcon.Show(false);
                        selectedIcon.SetAlpha(0);
                    }
                }
            }
            DestroyTooltips();
        }

        if (w && w == volumeBtn)
        {
            delete navBtnTooltipView;
            navBtnTooltipView = null;
        }
        return false;
    }

    void OnItemFavExecute()
    {
        SwitchFav();
        if (isFav)
            TraderXFavoritesService.GetInstance().AddFavorite(item);
        else
            TraderXFavoritesService.GetInstance().RemoveFavorite(item);
    }

    void UpdateVisuals()
    {
        if (selectedIcon) selectedIcon.Show(item.isSelected);
        if (unselectedIcon) unselectedIcon.Show(!item.isSelected);
        if (itemCardContent) itemCardContent.SetColor(TraderXViewStyles.ITEM_CARD_IDLE);
        if (itemCardOutline) itemCardOutline.SetAlpha(TernaryFloat(item.isSelected, 1.0, 0.0));

        if (tradeMode == ETraderXTradeMode.SELL && TraderXProductStockRepository.IsStockReached(item.productId, item.maxStock))
        {
            if (itemCardContent) itemCardContent.SetColor(TraderXViewStyles.ITEM_CARD_WARNING);
        }

        if (m_Root) m_Root.Update();
    }

    void SetItemSelected(bool selected)
    {
        TraderXSelectionService selectionService = TraderXSelectionService.GetInstance();
        if (selected)
            selectionService.SelectItem(item);
        else
            selectionService.DeselectItem(item);
    }

    void SwitchFav()
    {
        isFav = !isFav;
        UpdateFav();
    }

    void UpdateFav()
    {
        if (unfavIcon) unfavIcon.Show(!isFav);
        if (favIcon) favIcon.Show(isFav);
    }

    void OnItemSelectionChanged(TraderXProduct updatedItem)
    {
        if (!item || !updatedItem)
            return;

        if (item.productId == updatedItem.productId)
            UpdateVisuals();
    }

    void OnCheckoutPricingChanged()
    {
        if (tradeMode == ETraderXTradeMode.SELL)
        {
            string newPrice = GetPriceFromItem();
            if (newPrice != itemPrice)
            {
                itemPrice = newPrice;
                if (itemPriceText) itemPriceText.SetText(itemPrice);
            }
        }
    }

    void OnTraderXResponseReceived(int response, TraderXTransactionResultCollection transactionResultCollection = null)
    {
        if (response == ETraderXResponse.ALL_STOCK_RECEIVED || response == ETraderXResponse.TRANSACTIONS)
            UpdateStock();
    }
}
