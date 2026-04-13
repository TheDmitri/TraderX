class CatalogItemCardComponent extends DZlrComponentBase
{
    private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("traderx-catalog-item-card", CatalogItemCardComponent);

    static ref ScriptInvoker Event_OnCatalogItemCardClickCallBack = new ScriptInvoker();

    ref TraderXProduct item;
    string itemName, itemPrice, itemStock;
    EntityAI preview;
    int categoryType;

    Widget itemCardContent, stockText;
    ImageWidget healthImg;
    ButtonWidget itemCardMainBtn;
    TextWidget itemNameText, itemPriceText, itemStockText;
    ItemPreviewWidget itemPreviewWidget;

    void CatalogItemCardComponent(TraderXProduct item, int categoryType = ETraderXCategoryType.NONE)
    {
        this.item = item;
        this.categoryType = categoryType;
    }

    override protected void RegisterMetadata()
    {
        m_Metadata = new ComponentMetadata();
        m_Metadata.selector = "traderx-catalog-item-card";
        m_Metadata.templatePath = "TraderX/datasets/gui/TraderXPrefab/ItemCard/CatalogItemCardView.layout";
    }

    override void OnInit()
    {
        TraderXTradingService.Event_OnTraderXResponseReceived.Insert(OnTraderXResponseReceived);
        Setup();
    }

    override void OnDestroy()
    {
        TraderXTradingService.Event_OnTraderXResponseReceived.Remove(OnTraderXResponseReceived);
    }

    void Setup()
    {
        ShowName();
        UpdateStock();
        CreatePreview();
        itemPrice = GetSellPrice();
        if (itemNameText) itemNameText.SetText(itemName);
        if (itemPriceText) itemPriceText.SetText(itemPrice);
        if (itemPreviewWidget) itemPreviewWidget.SetItem(preview);
    }

    void ShowName() { itemName = item.GetDisplayName(); }

    void UpdateStock()
    {
        if (item.IsStockUnlimited())
        {
            if (stockText) stockText.Show(false);
        }
        else
        {
            itemStock = TraderXProductStockRepository.GetStockDisplay(item.productId, item.maxStock);
            if (stockText) stockText.Show(true);
            if (itemStockText) itemStockText.SetText(itemStock);
        }
    }

    void CreatePreview()
    {
        preview = EntityAI.Cast(GetGame().CreateObjectEx(item.className, vector.Zero, ECE_LOCAL | ECE_NOLIFETIME));
        LookForDefaultPreset();
        CreateAttachments();
    }

    void LookForDefaultPreset()
    {
        TraderXPreset defaultPreset = TraderXPresetsService.GetInstance().GetDefaultPreset(item.productId);
        if (!defaultPreset) defaultPreset = TraderXPresetsService.GetInstance().GetServerDefaultPreset(item.productId);
        if (!defaultPreset) return;
        item.defaultPreset = defaultPreset;
        item.selectedAttachments = defaultPreset.attachments;
    }

    void CreateAttachments()
    {
        if (!item || !item.selectedAttachments) return;
        foreach (string productId : item.selectedAttachments)
        {
            TraderXProduct attachment = TraderXProductRepository.GetItemById(productId);
            if (!attachment) continue;
            EntityAI entAttach = EntityAI.Cast(GetGame().CreateObjectEx(attachment.className, vector.Zero, ECE_LOCAL | ECE_NOLIFETIME));
            if (entAttach) TraderXAttachmentHandler.TryAttachItem(preview, entAttach);
        }
    }

    string GetSellPrice() { return TraderXPricingService.GetInstance().GetPricePreview(item.GetProductId(), false, 1, TraderXItemState.PRISTINE).ToString(); }
    int GetPrice() { return TraderXPricingService.GetInstance().GetPricePreview(item.GetProductId(), false, 1, TraderXItemState.PRISTINE); }
    TraderXProduct GetItem() { return item; }
    EntityAI GetPreview() { return preview; }

    string GetItemDescription()
    {
        if (!preview) return string.Empty;
        InventoryItem iItem = InventoryItem.Cast(preview);
        if (iItem) return TraderXCoreUtils.TrimUnt(iItem.GetTooltip());
        return string.Empty;
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (w != itemCardMainBtn) return false;
        if (button == MouseState.LEFT) { Event_OnCatalogItemCardClickCallBack.Invoke(this, ETraderXClickEvents.LCLICK); return true; }
        if (button == MouseState.RIGHT) { Event_OnCatalogItemCardClickCallBack.Invoke(this, ETraderXClickEvents.RCLICK); return true; }
        return false;
    }

    override bool OnDoubleClick(Widget w, int x, int y, int button)
    {
        if (button == MouseState.LEFT) { Event_OnCatalogItemCardClickCallBack.Invoke(this, ETraderXClickEvents.DOUBLE_CLICK); return true; }
        return false;
    }

    void OnTraderXResponseReceived(int response, TraderXTransactionResultCollection transactionResultCollection = null)
    {
        if (response == ETraderXResponse.ALL_STOCK_RECEIVED || response == ETraderXResponse.TRANSACTIONS)
            UpdateStock();
    }
}
