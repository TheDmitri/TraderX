class FavoritePageComponent extends DZlrComponentBase
{
    private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("traderx-favorite-page", FavoritePageComponent);

    string search_keyword;
    ref array<ref CategoryCardComponent> m_CategoryCards = new array<ref CategoryCardComponent>();
    private bool m_IsUpdatingFavorites = false;

    // Widgets
    Widget category_card_list;
    EditBoxWidget searchBox;

    override protected void RegisterMetadata()
    {
        m_Metadata = new ComponentMetadata();
        m_Metadata.selector = "traderx-favorite-page";
        m_Metadata.templatePath = "TraderX/datasets/gui/Favorites/FavoritesView.layout";
    }

    override void OnInit()
    {
        ItemCardComponent.Event_OnItemCardClickEventCallBack.Insert(OnItemCardClick);
        TraderXFavoritesService.Event_OnFavoriteChangedCallBack.Insert(OnFavoriteChanged);
        Setup();
    }

    override void OnDestroy()
    {
        ItemCardComponent.Event_OnItemCardClickEventCallBack.Remove(OnItemCardClick);
        TraderXFavoritesService.Event_OnFavoriteChangedCallBack.Remove(OnFavoriteChanged);
        ClearCategoryCards();
    }

    void Setup()
    {
        TraderXUINavigationService.GetInstance().SetNavigationId(ENavigationIds.FAVORITE);
        TraderXTradingService.GetInstance().SetTradeMode(ETraderXTradeMode.BUY);
        ShowFavoriteList();
    }

    void ShowFavoriteList()
    {
        ClearCategoryCards();
        array<ref TraderXCategory> categories = new array<ref TraderXCategory>();
        TraderXTradingService.GetInstance().GetFavoritesCategories(categories);
        foreach (TraderXCategory traderXCategory : categories)
        {
            CategoryCardComponent card = new CategoryCardComponent();
            card.InitComponent(null);
            card.SetCategoryCardData(traderXCategory, EItemCardSize.LARGE, true, ETraderXCategoryType.NONE);
            AddCategoryCard(card);
        }
    }

    void ClearCategoryCards()
    {
        foreach (CategoryCardComponent card : m_CategoryCards)
        {
            if (card && card.GetRoot())
                card.GetRoot().Unlink();
        }
        m_CategoryCards.Clear();
    }

    void AddCategoryCard(CategoryCardComponent card)
    {
        if (card && category_card_list)
        {
            category_card_list.AddChild(card.GetRoot());
            card.GetRoot().Show(true);
        }
        m_CategoryCards.Insert(card);
    }

    void UpdateCategoryList()
    {
        foreach (CategoryCardComponent categoryCard : m_CategoryCards)
        {
            categoryCard.FilterList(search_keyword);
        }
    }

    void DebouncedSearch()
    {
        UpdateCategoryList();
    }

    override bool OnChange(Widget w, int x, int y, bool finished)
    {
        if (w == searchBox)
        {
            search_keyword = searchBox.GetText();
            GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(DebouncedSearch);
            GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(DebouncedSearch, 300, false);
            return true;
        }
        return false;
    }

    void OnItemCardClick(ItemCardComponent itemSelected, int command = ETraderXClickEvents.LCLICK)
    {
        switch (command)
        {
            case ETraderXClickEvents.RCLICK:
                OnItemCardRClick(itemSelected);
                break;
            case ETraderXClickEvents.DOUBLE_CLICK:
                OnItemCardDoubleClick(itemSelected);
                break;
        }
    }

    void OnItemCardRClick(ItemCardComponent itemSelected)
    {
        if (!itemSelected.GetItem().HasVariants() && !itemSelected.GetItem().HasAttachments())
            return;

        CustomizePageComponent customizePage = new CustomizePageComponent(itemSelected.GetItem());
        customizePage.InitComponent(null);
        TraderXMainComponent.GetInstance().AddSubPage(customizePage);
    }

    void OnItemCardDoubleClick(ItemCardComponent itemSelected)
    {
        if (TraderXTradingService.GetInstance().IsTransactionPending())
            return;

        int npcId = TraderXTradingService.GetInstance().GetNpcId();
        TraderXTransactionCollection transactionCollection = new TraderXTransactionCollection();
        transactionCollection.AddTransaction(TraderXTransaction.CreateBuyTransaction(itemSelected.item, 1, itemSelected.GetPrice(), npcId));

        TraderXTradingService.GetInstance().LockTransaction();
        GetDayZularRPCManager().SendRPC("TraderX", "GetTransactionsRequest", new Param2<TraderXTransactionCollection, int>(transactionCollection, npcId));
    }

    void OnFavoriteChanged(TraderXFavoritesService favoritesService)
    {
        ShowFavoriteList();
    }
}
