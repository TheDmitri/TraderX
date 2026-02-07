class ItemCardViewController: ViewController
{
    ref TraderXProduct item;
    string itemName, itemPrice, itemStock;
    float itemVolume;
    EntityAI preview;
    int tradeMode;
    int categoryType;
    bool selectable, isFavable, isFav;
    Widget itemCardOutline, itemCardContent, stockText;
	ImageWidget selectedIcon, unselectedIcon, favIcon, unfavIcon, healthImg;
    Widget volumeBar, favBtn;
    ButtonWidget itemCardMainBtn, volumeBtn;

    ref NavBtnTooltipView  navBtnTooltipView;

    static ref ScriptInvoker Event_OnItemCardClickEventCallBack = new ScriptInvoker();
    static ref ScriptInvoker Event_OnItemCardSelectCallBack = new ScriptInvoker();
    static ref ScriptInvoker Event_OnDestroyAllTooltips = new ScriptInvoker();

    ref SellTooltipView sellTooltipView;
    ref BuyTooltipView buyTooltipView;

    ref CustomizeTooltipView customizeTooltipView;

    string quantityStr;

    void ItemCardViewController()
    {
        TraderXSelectionService.Event_OnItemSelectionChanged.Insert(OnItemSelectionChanged);
        TraderXTradingService.Event_OnTraderXResponseReceived.Insert(OnTraderXResponseReceived);
        // Subscribe to checkout pricing changes for dynamic price updates
        TraderXProduct.Event_OnCheckoutPricingChanged.Insert(OnCheckoutPricingChanged);
        Event_OnDestroyAllTooltips.Insert(DestroyTooltips);
    }

    void ~ItemCardViewController()
    {
        Event_OnDestroyAllTooltips.Remove(DestroyTooltips);
        DestroyTooltips();
    }

    static void DestroyAllTooltips()
    {
        Event_OnDestroyAllTooltips.Invoke();
    }

    void Setup(TraderXProduct item, int categoryType, bool selectable, bool isFavable, bool isFav)
    {
        this.item = item;
        this.tradeMode = TraderXTradingService.GetInstance().GetTradeMode();
        this.categoryType = categoryType;
        this.selectable = selectable;
        this.isFavable = isFavable;

        if(isFavable){
            this.isFav = TraderXFavoritesService.GetInstance().IsFavorite(item);
            UpdateFav();
        }

        ShowName();
        UpdateStock();
        bool showBar = GetItemVolumePerCent(itemVolume); 
        volumeBar.Show(showBar);

        CreatePreview();

        itemPrice = GetPriceFromItem();

        SetItemHealth();

        if(item.isSelected){
            Event_OnItemCardSelectCallBack.Invoke(this);
        }
        
        NotifyPropertiesChanged({"itemName", "itemPrice", "itemVolume"});
        OnShow(true);
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
        if(item.IsStockUnlimited()){
            stockText.Show(false);
            return;
        }
        else{
            GetTraderXLogger().LogDebug("UpdateStock - Product: " + item.productId + " maxStock: " + item.maxStock);
        }

        itemCardContent.SetColor(TraderXViewStyles.ITEM_CARD_IDLE);
        selectable = true; // Reset to selectable by default
        
        // Handle stock validation for sell mode
        if(tradeMode == ETraderXTradeMode.SELL) {
            if(TraderXProductStockRepository.IsStockReached(item.productId, item.maxStock)) {
                stockText.SetColor(ARGB(255, 255, 0, 0));
                selectable = false;
            }
        }
        
        // Handle stock validation for buy mode
        if(tradeMode == ETraderXTradeMode.BUY) {
            if(TraderXProductStockRepository.GetStockValue(item.productId) == 0) {
                stockText.SetColor(ARGB(255, 255, 0, 0));
                selectable = false;
            }
        }
            
        itemStock = TraderXProductStockRepository.GetStockDisplay(item.productId, item.maxStock);
        NotifyPropertyChanged("itemStock");
    }

    void SetItemHealth()
    {
        int a,r,g,b;
        if(tradeMode == ETraderXTradeMode.SELL){
            ItemBase itemB = ItemBase.Cast(GetGame().GetObjectByNetworkId(item.playerItem.networkIdLow, item.playerItem.networkIdHigh));
            if(!itemB)
                return;

            float health = itemB.GetHealthLevel();
            TraderXCoreUtils.GetHealthFromLevel(health, a,r,g,b);
            healthImg.SetColor(ARGB(a,r,g,b));
        }
    }

    bool IsPreviewAlive()
    {
        return GetGame().GetObjectByNetworkId(item.playerItem.networkIdLow, item.playerItem.networkIdHigh) != null;
    }

    void CreatePreview()
    {
        if(tradeMode == ETraderXTradeMode.SELL){
            preview = GetGame().GetObjectByNetworkId(item.playerItem.networkIdLow, item.playerItem.networkIdHigh);
            if(!preview)
                preview = EntityAI.Cast(GetGame().CreateObjectEx(item.className, vector.Zero, ECE_LOCAL|ECE_NOLIFETIME));
        }
        else
        {
            preview = EntityAI.Cast(GetGame().CreateObjectEx(item.className, vector.Zero, ECE_LOCAL|ECE_NOLIFETIME));
            
            // Check for default preset and apply it
            LookForDefaultPreset();
            
            // Create attachments (either from preset or selected attachments)
            CreateAttachments();
        }

        NotifyPropertyChanged("preview");
    }

    void LookForDefaultPreset()
    {
        TraderXPreset defaultPreset = TraderXPresetsService.GetInstance().GetDefaultPreset(item.productId);
        if (!defaultPreset)
        {
            defaultPreset = TraderXPresetsService.GetInstance().GetServerDefaultPreset(item.productId);
        }
        
        if (!defaultPreset)
            return;
        
        item.defaultPreset = defaultPreset;
        
        // Set selected attachments from preset
        item.selectedAttachments = defaultPreset.attachments;
    }

    void CreateAttachments()
    {
        if(!item|| item && !item.selectedAttachments)
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

    EntityAI GetPreview()
    {
        return preview;
    }

    void UpdatePreview()
    {
        NotifyPropertyChanged("preview");
    }

    string GetPriceFromItem()
    {
        if(tradeMode == ETraderXTradeMode.SELL){
            if(item.GetPlayerItem()){
                // Use dynamic sell pricing based on current stock and coefficient
                int dynamicPrice = TraderXPricingService.GetInstance().GetPricePreview(item.GetProductId(), false, 1, item.GetPlayerItem().healthLevel);
                return dynamicPrice.ToString();
            } else {
                // Catalog item - calculate theoretical sell price based on pristine condition
                int catalogSellPrice = TraderXPricingService.GetInstance().GetPricePreview(item.GetProductId(), false, 1, TraderXItemState.PRISTINE);
                return catalogSellPrice.ToString();
            }
        }

        if(tradeMode == ETraderXTradeMode.BUY){
            // Use dynamic buy pricing based on current stock and coefficient
            int basePrice = TraderXPricingService.GetInstance().GetPricePreview(item.GetProductId(), true, 1, TraderXItemState.PRISTINE);
            
            // Add preset price if default preset is applied
            if(item.defaultPreset)
            {
                int presetPrice = TraderXPresetsService.GetInstance().CalculateTotalPricePreset(item.defaultPreset);
                basePrice = presetPrice;
            }
            else
            {
                // Fallback to individual attachment pricing
                basePrice += CalculateAttachmentsPrice();
            }
            
            return basePrice.ToString();
        }

        return string.Empty;
    }

    int CalculateAttachmentsPrice()
    {
        int price = 0;

        if(!item.selectedAttachments)
            return price;
            
        foreach(UUID productId: item.selectedAttachments)
        {
            TraderXProduct attachment = TraderXProductRepository.GetItemById(productId);
            if(!attachment)
                continue;

            // Use dynamic pricing for attachments too
            price += TraderXPricingService.GetInstance().GetPricePreview(attachment.GetProductId(), true, 1, TraderXItemState.PRISTINE);
        }

        return price;
    }

    bool GetItemVolumePerCent(out float volume)
    {
        ItemBase itemB;
        if(tradeMode == ETraderXTradeMode.SELL)
        {
            itemB = ItemBase.Cast(GetGame().GetObjectByNetworkId(item.playerItem.networkIdLow, item.playerItem.networkIdHigh));
            if(!itemB)
                return false;

            if(!itemB.HasQuantity())
                return false;

            volume = TraderXQuantityManager.GetItemVolumePerCent(itemB);
            return true;
        }

        if(tradeMode == ETraderXTradeMode.BUY)
        {
            if(!preview)
                return false;
                
            itemB = ItemBase.Cast(preview);
            if(!itemB)
                return false;
                
            if(!itemB.HasQuantity())
                return false;
            
            // Get trade quantity limits from TraderXTradeQuantity
            int tradeQuantity = TraderXTradeQuantity.GetItemBuyQuantityClient(preview, item.tradeQuantity);
            int maxQuantity = TraderXQuantityManager.GetMaxItemQuantityClient(itemB);
            
            if(maxQuantity <= 0)
                return false;

            quantityStr = tradeQuantity.ToString() + "/" + maxQuantity;
                
            volume = Math.Round((tradeQuantity * 100) / maxQuantity);
            return true;
        }

        return false;
    }

    void GetTradeQtyAndMaxQty(out int tradeQuantity, out int maxQuantity)
    {
        if(!preview){
            return;
        }
                
        ItemBase itemB = ItemBase.Cast(preview);
        if(!itemB){
            return;
        }
                
        if(!itemB.HasQuantity()){
            return;
        }
            
        // Get trade quantity limits from TraderXTradeQuantity
        int tradeQuantity = TraderXTradeQuantity.GetItemBuyQuantityClient(preview, item.tradeQuantity);
        int maxQuantity = TraderXQuantityManager.GetMaxItemQuantityClient(itemB);
    }
        
    bool GetItemVolume(out float volume)
    {
        volume = GetItemVolumePerCent(volume)/100;  
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
        if(!preview)
            return string.Empty;

        InventoryItem iItem = InventoryItem.Cast(preview);
        if(iItem)
            return TraderXCoreUtils.TrimUnt(iItem.GetTooltip());

        return string.Empty;
    }

    void OnShow(bool show)
    {
        if (show)
        {
            favIcon.Show(isFav && isFavable);
            unfavIcon.Show(!isFav && isFavable);
            selectedIcon.Show(item.isSelected && selectable);
            itemCardOutline.SetAlpha(Ternary<float>.If(item.isSelected, 1.0, 0.0));
            unselectedIcon.Show(!item.isSelected && selectable);
            healthImg.Show(tradeMode == ETraderXTradeMode.SELL);
        }
        else
        {
            DestroyTooltips();
        }
    }

    override bool OnClick(Widget w, int x, int y, int button)
	{
        if(w == favBtn){
            OnItemFavExecute();
            return true;
        }

		if (w != itemCardMainBtn) {
            return false;
        }

        if(!selectable)
            return false;

        if(button == MouseState.LEFT){
            if(KeyState(KeyCode.KC_LSHIFT) == 0){
                SetItemSelected(!item.isSelected);
            }   

            Event_OnItemCardClickEventCallBack.Invoke(this, Ternary<int>.If(KeyState(KeyCode.KC_LSHIFT) != 0, ETraderXClickEvents.SHIFT_LCLICK, ETraderXClickEvents.LCLICK));
            return true;
        }

        if(button == MouseState.RIGHT){
            Event_OnItemCardClickEventCallBack.Invoke(this, Ternary<int>.If(KeyState(KeyCode.KC_LSHIFT) != 0, ETraderXClickEvents.SHIFT_RCLICK, ETraderXClickEvents.RCLICK));
            return true;
        }
		
		return false;
	}
	
    override bool OnMouseWheel(Widget w, int x, int y, int wheel)
    {        
        return false;
    }

    override bool OnDoubleClick(Widget w, int x, int y, int button)
	{
        if(!selectable)
            return false;

        if (button == MouseState.LEFT){
            Event_OnItemCardClickEventCallBack.Invoke(this, ETraderXClickEvents.DOUBLE_CLICK);
            return true;
        }

        return false;          
	}

    override bool OnMouseEnter(Widget w, int x, int y)
	{
		if(w && w.GetUserID() == EButtonIds.SELECTED && selectable)
        {
            if(!item.isSelected){
                itemCardOutline.SetAlpha(0.5);
                if (selectable)
                {
                    unselectedIcon.Show(false);
                    selectedIcon.Show(true);
                    selectedIcon.SetAlpha(0.5);
                }
            }

            ShowTooltip();
		}

        if(w && w.GetUserID() == 102){
            if(quantityStr == string.Empty)
                return false;
            
            navBtnTooltipView = new NavBtnTooltipView(quantityStr, 10.0, 10.0);
            return true;
        }
		return true;
	}

    void ShowTooltip()
    {
        if(!preview)
            return;

        if(tradeMode == ETraderXTradeMode.BUY && categoryType == ETraderXCategoryType.NONE){
            buyTooltipView = new BuyTooltipView(GetItemDescription(), item.HasVariants(), item.HasAttachments(), -0.273438, -0.104167);
            buyTooltipView.Show(true);
            return;
        }

        if(tradeMode == ETraderXTradeMode.BUY && categoryType != ETraderXCategoryType.NONE){
            customizeTooltipView = new CustomizeTooltipView(GetItemDescription());
            customizeTooltipView.Show(true);
            return;
        }

        if(tradeMode == ETraderXTradeMode.SELL)
        {
            sellTooltipView = new SellTooltipView(GetItemDescription(), TraderXInventoryManager.GetEntityHierarchy(preview), -0.273438, -0.104167);
            sellTooltipView.Show(true);
            return;
        }
    }

    void DestroyTooltips()
    {
        if(buyTooltipView)
            delete buyTooltipView;

        if(sellTooltipView)
            delete sellTooltipView;

        if(customizeTooltipView)
            delete customizeTooltipView;
    }

	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		if(w && w.GetUserID() == EButtonIds.SELECTED && selectable)
        {
            if(!item.isSelected){
                itemCardOutline.SetAlpha(0.0);
                if (selectable)
                {
                    unselectedIcon.Show(true);
                    selectedIcon.Show(false);
                    selectedIcon.SetAlpha(0);
                }
            }

            DestroyTooltips();
		}

        if(w && w == volumeBtn){
            delete navBtnTooltipView;
        }
		return false;
	}

    bool OnItemFavExecute()
    {
        SwitchFav();
        if (isFav){
			TraderXFavoritesService.GetInstance().AddFavorite(item);
			return true;
		}
            
        TraderXFavoritesService.GetInstance().RemoveFavorite(item);

		return true;
    }

    void UpdateVisuals()
    {
        if(selectedIcon) selectedIcon.Show(item.isSelected);
        if(unselectedIcon) unselectedIcon.Show(!item.isSelected);
        if(itemCardContent) itemCardContent.SetColor(TraderXViewStyles.ITEM_CARD_IDLE);
        if(itemCardOutline) itemCardOutline.SetAlpha(Ternary<float>.If(item.isSelected, 1.0, 0.0));

        if (tradeMode == ETraderXTradeMode.SELL && TraderXProductStockRepository.IsStockReached(item.productId, item.maxStock))
        {
            itemCardContent.SetColor(TraderXViewStyles.ITEM_CARD_WARNING);
        }

        // Force a layout update to ensure all visual changes are applied
        GetLayoutRoot().Update();
    }

    void SetItemSelected(bool selected)
    {
        TraderXSelectionService selectionService = TraderXSelectionService.GetInstance();
        
        if (selected)
        {
            selectionService.SelectItem(item);
        }
        else
        {
            selectionService.DeselectItem(item);
        }
    }

    void SwitchFav()
    {
        //Store le fav
        isFav = !isFav;
        UpdateFav();
    }

    void UpdateFav()
    {
        if(unfavIcon) unfavIcon.Show(!isFav);
        if(favIcon) favIcon.Show(isFav);
    }

    void SetPositionTooltip()
	{
		int x, y;
		GetMousePos(x, y);
		int w, h;
		GetScreenSize(w, h);
		GetLayoutRoot().SetPos(x + 10.0, y - 10.0, true);
		GetLayoutRoot().Update();
	}

    void OnItemSelectionChanged(TraderXProduct updatedItem)
    {
        if(!item || !updatedItem)
            return;
        
        if (item.productId == updatedItem.productId){
            UpdateVisuals();
        }
    }

    void OnCheckoutPricingChanged()
    {
        // Update item card price when checkout pricing changes (for sell mode dynamic pricing)
        if (tradeMode == ETraderXTradeMode.SELL) {
            string newPrice = GetPriceFromItem();
            if (newPrice != itemPrice) {
                itemPrice = newPrice;
                NotifyPropertyChanged("itemPrice");
                GetTraderXLogger().LogDebug(string.Format("[ITEM_CARD] Price updated for %1: %2", item.GetProductId(), itemPrice));
            }
        }
    }

    void OnTraderXResponseReceived(int response, TraderXTransactionResultCollection transactionResultCollection = null)
    {
        if(response == ETraderXResponse.ALL_STOCK_RECEIVED)
        {
            UpdateStock();
        }
        else if(response == ETraderXResponse.TRANSACTIONS)
        {
            UpdateStock();
        }
    }
}