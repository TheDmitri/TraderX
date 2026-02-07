class PurchasePageViewController: ViewController
{
    string search_keyword;
    ref ObservableCollection<ref CategoryCardView> category_card_list = new ObservableCollection<ref CategoryCardView>(this);
    bool m_AllExpanded = false;
    TextWidget toggleExpandText;
    
    void PurchasePageViewController()
    {
        ItemCardViewController.Event_OnItemCardClickEventCallBack.Insert(OnItemCardClick);
    }

    void Setup()
    {
        TraderXUINavigationService.GetInstance().SetNavigationId(ENavigationIds.PURCHASE);
        TraderXTradingService.GetInstance().SetTradeMode(ETraderXTradeMode.BUY);
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
                
            category_card_list.Insert(new CategoryCardLargeView(category, EItemCardSize.LARGE, false));
        }
    }

    void UpdateCategoryList()
    {
        array<ref CategoryCardView> categoryCards = category_card_list.GetArray();
        foreach(CategoryCardView categoryCard: categoryCards)
        {
            categoryCard.GetTemplateController().FilterList(search_keyword);
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
        array<ref CategoryCardView> cards = category_card_list.GetArray();
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

    void OnItemCardClick(ItemCardViewController itemSelected, int command = ETraderXClickEvents.LCLICK)
    {
        switch(command)
        {
            case ETraderXClickEvents.RCLICK:
                OnItemCardRClick(itemSelected);
            break;
                
            case ETraderXClickEvents.DOUBLE_CLICK:
                OnItemCardDoubleClick(itemSelected);
            break;

            default:
                GetTraderXLogger().LogWarning("Unknown item card click event");
                break;
        }
    }

    void OnItemCardRClick(ItemCardViewController itemSelected)
    {
        if(!itemSelected.GetItem().HasVariants() && !itemSelected.GetItem().HasAttachments())
            return;

        TraderXMainViewController.GetInstance().AddSubPage(new CustomizePageView(itemSelected.GetItem()));
    }

    void OnItemCardDoubleClick(ItemCardViewController itemSelected)
    {
        if(TraderXTradingService.GetInstance().IsTransactionPending())
            return;

        int npcId = TraderXTradingService.GetInstance().GetNpcId();
        TraderXTransactionCollection transactionCollection = new TraderXTransactionCollection();
        transactionCollection.AddTransaction(TraderXTransaction.CreateBuyTransaction(itemSelected.item, 1, itemSelected.GetPrice(), npcId));
        
        TraderXTradingService.GetInstance().LockTransaction();
        GetRPCManager().SendRPC("TraderX", "GetTransactionsRequest", new Param2<TraderXTransactionCollection, int>(transactionCollection, npcId));
        
        GetTraderXLogger().LogDebug(string.Format("Transaction request sent for item %1 with price %2", itemSelected.item.className, itemSelected.GetPrice()));
    }
}