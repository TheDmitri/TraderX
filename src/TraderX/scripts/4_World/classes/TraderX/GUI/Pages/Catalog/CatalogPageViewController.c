class CatalogPageViewController: ViewController
{
    string search_keyword;
    ref ObservableCollection<ref CatalogCategoryCardView> category_card_list = new ObservableCollection<ref CatalogCategoryCardView>(this);
    bool m_AllExpanded = false;
    TextWidget toggleExpandText;
    
    // Selected item details
    string selected_item_name;
    string selected_sell_price;
    string selected_buy_price;
    string selected_stock;
    EntityAI selected_preview;
    string selected_description;
    
    ref TraderXProduct selected_item;
    
    void CatalogPageViewController()
    {
        CatalogItemCardViewController.Event_OnCatalogItemCardClickCallBack.Insert(OnCatalogItemCardClick);
    }

    void Setup()
    {
        TraderXUINavigationService.GetInstance().SetNavigationId(ENavigationIds.CATALOG);
        TraderXTradingService.GetInstance().SetTradeMode(ETraderXTradeMode.SELL);
        ShowCategoryList();
    }

    void ShowCategoryList()
    {
        category_card_list.Clear();
        array<ref TraderXCategory> categories = TraderXTradingService.GetInstance().GetTraderCategories();
        foreach (TraderXCategory category: categories)
        {
            if(!category || (category && !category.isVisible))
                continue;
            
            // Only show categories with products that can be sold
            if (!HasSellableProducts(category))
                continue;
                
            category_card_list.Insert(new CatalogCategoryCardView(category, EItemCardSize.LARGE, false, ETraderXCategoryType.NONE));
        }
    }
    
    bool HasSellableProducts(TraderXCategory category)
    {
        array<ref TraderXProduct> products = category.GetProducts();
        foreach (TraderXProduct product : products)
        {
            if (product && product.CanBeSold())
                return true;
        }
        return false;
    }

    void UpdateCategoryList()
    {
        array<ref CatalogCategoryCardView> categoryCards = category_card_list.GetArray();
        foreach(CatalogCategoryCardView categoryCard: categoryCards)
        {
            CatalogCategoryCardViewController controller = categoryCard.GetTemplateController();
            controller.FilterList(search_keyword);
        }
    }

    void DebouncedSearch()
    {
        UpdateCategoryList();
    }

    override void PropertyChanged(string property_name)
	{
		switch (property_name)
		{
			case "search_keyword": 
			{
				GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(DebouncedSearch);
				GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(DebouncedSearch, 300, false);
				break;
			}
		}
	}

    bool OnToggleExpandExecute(ButtonCommandArgs args)
    {
        TraderXInventoryManager.PlayMenuSound(ETraderXSounds.SELECT);
        m_AllExpanded = !m_AllExpanded;
        array<ref CatalogCategoryCardView> cards = category_card_list.GetArray();
        for(int i = 0; i < cards.Count(); i++)
        {
            cards[i].GetTemplateController().SetExpanded(!m_AllExpanded);
        }
        if(toggleExpandText)
        {
            if(m_AllExpanded)
                toggleExpandText.SetText(Widget.TranslateString("#tpm_collapse_all"));
            else
                toggleExpandText.SetText(Widget.TranslateString("#tpm_expand_all"));
        }
        return true;
    }

    void OnCatalogItemCardClick(CatalogItemCardViewController itemSelected, int command = ETraderXClickEvents.LCLICK)
    {
        switch(command)
        {
            case ETraderXClickEvents.LCLICK:
                GetTraderXLogger().LogDebug(string.Format("[CATALOG] Item selected: %1", itemSelected.GetItem().className));
                UpdateSelectedItemDetails(itemSelected);
            break;
                
            case ETraderXClickEvents.RCLICK:
                OnCatalogItemCardRClick(itemSelected);
            break;

            default:
                GetTraderXLogger().LogWarning("Unknown item card click event");
                break;
        }
    }
    
    void UpdateSelectedItemDetails(CatalogItemCardViewController itemSelected)
    {
        selected_item = itemSelected.GetItem();
        
        // Update item name
        selected_item_name = selected_item.GetDisplayName();
        
        // Update preview
        selected_preview = itemSelected.GetPreview();
        
        // Calculate prices
        int sellPrice = TraderXPricingService.GetInstance().GetPricePreview(selected_item.GetProductId(), false, 1, TraderXItemState.PRISTINE);
        selected_sell_price = sellPrice.ToString();
        
        int buyPrice = TraderXPricingService.GetInstance().GetPricePreview(selected_item.GetProductId(), true, 1, TraderXItemState.PRISTINE);
        // Add preset price if applicable
        if(selected_item.defaultPreset)
        {
            int presetPrice = TraderXPresetsService.GetInstance().CalculateTotalPricePreset(selected_item.defaultPreset);
            buyPrice = presetPrice;
        }
        selected_buy_price = buyPrice.ToString();
        
        // Update stock
        selected_stock = TraderXProductStockRepository.GetStockDisplay(selected_item.productId, selected_item.maxStock);
        
        // Update description
        selected_description = itemSelected.GetItemDescription();
        if(selected_description == string.Empty)
            selected_description = "#tpm_no_description_available";
        
        NotifyPropertiesChanged({"selected_item_name", "selected_preview", "selected_sell_price", "selected_buy_price", "selected_stock", "selected_description"});
    }

    void OnCatalogItemCardRClick(CatalogItemCardViewController itemSelected)
    {
        if(!itemSelected.GetItem().HasVariants() && !itemSelected.GetItem().HasAttachments())
            return;

        TraderXMainViewController.GetInstance().AddSubPage(new CustomizePageView(itemSelected.GetItem()));
    }
}
