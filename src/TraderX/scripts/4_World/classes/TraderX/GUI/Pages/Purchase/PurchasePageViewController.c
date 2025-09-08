class PurchasePageViewController: ViewController
{
    string search_keyword;
    ref ObservableCollection<ref CategoryCardView> category_card_list = new ObservableCollection<ref CategoryCardView>(this);
    
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

    override void PropertyChanged(string property_name)
	{
		switch (property_name)
		{
			case "search_keyword": 
			{
				UpdateCategoryList();
				break;
			}
		}
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
        int npcId = TraderXTradingService.GetInstance().GetNpcId();
        TraderXTransactionCollection transactionCollection = new TraderXTransactionCollection();
        transactionCollection.AddTransaction(TraderXTransaction.CreateBuyTransaction(itemSelected.item, 1, itemSelected.GetPrice(), npcId));
        
        // Envoyer la requête via RPC    
        GetRPCManager().SendRPC("TraderX", "GetTransactionsRequest", new Param2<TraderXTransactionCollection, int>(transactionCollection, npcId));
        
        GetTraderXLogger().LogDebug(string.Format("Transaction request sent for item %1 with price %2", itemSelected.item.className, itemSelected.GetPrice()));
    }
}