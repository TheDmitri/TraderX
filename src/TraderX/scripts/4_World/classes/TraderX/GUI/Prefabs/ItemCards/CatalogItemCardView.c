class CatalogItemCardViewController: ViewController
{
    ref TraderXProduct item;
    string itemName, itemPrice, itemStock;
    EntityAI preview;
    int categoryType;
    
    Widget itemCardContent, stockText;
    ImageWidget healthImg;
    ButtonWidget itemCardMainBtn;

    static ref ScriptInvoker Event_OnCatalogItemCardClickCallBack = new ScriptInvoker();

    void CatalogItemCardViewController()
    {
        TraderXTradingService.Event_OnTraderXResponseReceived.Insert(OnTraderXResponseReceived);
    }

    void Setup(TraderXProduct item, int categoryType, bool selectable = false, bool isFavable = false, bool isFav = false)
    {
        this.item = item;
        this.categoryType = categoryType;

        ShowName();
        UpdateStock();
        CreatePreview();
        itemPrice = GetSellPrice();

        NotifyPropertiesChanged({"itemName", "itemPrice", "itemStock"});
        OnShow(true);
    }

    void ShowName()
    {
        itemName = item.GetDisplayName();
    }

    void UpdateStock()
    {
        if(item.IsStockUnlimited()){
            stockText.Show(false);
        } else {
            itemStock = TraderXProductStockRepository.GetStockDisplay(item.productId, item.maxStock);
            stockText.Show(true);
        }
        NotifyPropertyChanged("itemStock");
    }

    void CreatePreview()
    {
        // Always create preview from className for catalog items (no player item needed)
        preview = EntityAI.Cast(GetGame().CreateObjectEx(item.className, vector.Zero, ECE_LOCAL|ECE_NOLIFETIME));
        
        // Apply default preset if available
        LookForDefaultPreset();
        CreateAttachments();

        NotifyPropertyChanged("preview");
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
        if(!item || !item.selectedAttachments)
            return;

        foreach(string productId: item.selectedAttachments)
        {
            TraderXProduct attachment = TraderXProductRepository.GetItemById(productId);
            if(!attachment)
                continue;

            EntityAI entAttach = EntityAI.Cast(GetGame().CreateObjectEx(attachment.className, vector.Zero, ECE_LOCAL|ECE_NOLIFETIME));
            if(!entAttach)
                continue;

            TraderXAttachmentHandler.TryAttachItem(preview, entAttach);
        }
    }

    string GetSellPrice()
    {
        // Calculate theoretical sell price based on pristine condition
        int sellPrice = TraderXPricingService.GetInstance().GetPricePreview(item.GetProductId(), false, 1, TraderXItemState.PRISTINE);
        return sellPrice.ToString();
    }

    int GetPrice()
    {
        // Calculate theoretical sell price based on pristine condition
        return TraderXPricingService.GetInstance().GetPricePreview(item.GetProductId(), false, 1, TraderXItemState.PRISTINE);
    }

    TraderXProduct GetItem()
    {
        return item;
    }

    EntityAI GetPreview()
    {
        return preview;
    }

    void OnShow(bool show)
    {
        if (show)
        {
            // No selection UI for catalog items
        }
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (w != itemCardMainBtn)
            return false;

        if(button == MouseState.LEFT){
            Event_OnCatalogItemCardClickCallBack.Invoke(this, ETraderXClickEvents.LCLICK);
            return true;
        }

        if(button == MouseState.RIGHT){
            Event_OnCatalogItemCardClickCallBack.Invoke(this, ETraderXClickEvents.RCLICK);
            return true;
        }
        
        return false;
    }

    override bool OnDoubleClick(Widget w, int x, int y, int button)
    {
        if (button == MouseState.LEFT){
            Event_OnCatalogItemCardClickCallBack.Invoke(this, ETraderXClickEvents.DOUBLE_CLICK);
            return true;
        }
        return false;
    }

    string GetItemDescription()
    {
        if(!preview)
            return string.Empty;

        InventoryItem iItem = InventoryItem.Cast(preview);
        if(iItem)
            return TraderXCoreUtils.TrimUnt(iItem.GetTooltip());

        return string.Empty;
    }

    void OnTraderXResponseReceived(int response, TraderXTransactionResultCollection transactionResultCollection = null)
    {
        if(response == ETraderXResponse.ALL_STOCK_RECEIVED || response == ETraderXResponse.TRANSACTIONS)
        {
            UpdateStock();
        }
    }
}

class CatalogItemCardView: ScriptViewTemplate<CatalogItemCardViewController>
{
    void CatalogItemCardView(TraderXProduct item, int categoryType = ETraderXCategoryType.NONE, bool selectable = false, bool isFavable = false, bool isFav = false)
    {
        m_TemplateController.Setup(item, categoryType, selectable, isFavable, isFav);
    }

    override string GetLayoutFile() 
    {
        return "TraderX/datasets/gui/TraderXPrefab/ItemCard/CatalogItemCardView.layout";
    }

    override void Show(bool show)
    {
        m_TemplateController.OnShow(show);
        super.Show(show);
    }
}
