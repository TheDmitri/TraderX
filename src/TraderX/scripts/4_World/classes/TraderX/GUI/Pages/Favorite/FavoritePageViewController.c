class FavoritePageViewController: ViewController
{
    string search_keyword;
    ref ObservableCollection<ref CategoryCardView> category_card_list = new ObservableCollection<ref CategoryCardView>(this);
    private bool m_IsUpdatingFavorites = false;
    
    void FavoritePageViewController()
    {
        ItemCardViewController.Event_OnItemCardClickEventCallBack.Insert(OnItemCardClick);
        TraderXFavoritesService.Event_OnFavoriteChangedCallBack.Insert(OnFavoriteChanged);
    }

    void Setup()
    {
        TraderXUINavigationService.GetInstance().SetNavigationId(ENavigationIds.FAVORITE);
        TraderXTradingService.GetInstance().SetTradeMode(ETraderXTradeMode.BUY);
        ShowFavoriteList();
    }

    void ShowFavoriteList()
    {
        category_card_list.Clear();
        array<ref TraderXCategory> categories = new array<ref TraderXCategory>();
        TraderXTradingService.GetInstance().GetFavoritesCategories(categories);
        foreach (TraderXCategory traderXCategory: categories)
        {
            category_card_list.Insert(new CategoryCardLargeView(traderXCategory, EItemCardSize.LARGE, true, ETraderXTradeMode.BUY));
        }
    }

    void HideFavoriteCardList()
    {
        //category_card_list.Get(0).GetTemplateController().
    }

    void UpdateCategoryList()
    {
        foreach(CategoryCardView categoryCard: category_card_list.GetArray())
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
        
        GetTraderXLogger().LogDebug(string.Format("Transaction request sent for item %1 with price %2 and id: %3", itemSelected.item.className, itemSelected.GetPrice(), itemSelected.item.productId));
    }

    void OnFavoriteChanged(TraderXFavoritesService favoritesService)
    {
        //HideFavoriteCardList();
        ShowFavoriteList();
    }
}