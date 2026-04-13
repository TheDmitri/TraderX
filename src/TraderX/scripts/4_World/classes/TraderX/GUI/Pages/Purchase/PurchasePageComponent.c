class PurchasePageComponent extends DZlrComponentBase
{
    private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("traderx-purchase-page", PurchasePageComponent);

    string search_keyword;
    ref array<ref CategoryCardComponent> m_CategoryCards = new array<ref CategoryCardComponent>();
    bool m_AllExpanded = false;

    // Widgets
    Widget category_card_list;
    TextWidget toggleExpandText;
    ButtonWidget toggleExpandBtn;
    EditBoxWidget searchBox;

    override protected void RegisterMetadata()
    {
        m_Metadata = new ComponentMetadata();
        m_Metadata.selector = "traderx-purchase-page";
        m_Metadata.templatePath = "TraderX/datasets/gui/PurchasePage/PurchasePageView.layout";
    }

    override void OnInit()
    {
        ItemCardComponent.Event_OnItemCardClickEventCallBack.Insert(OnItemCardClick);
        Setup();
    }

    override void OnDestroy()
    {
        ItemCardComponent.Event_OnItemCardClickEventCallBack.Remove(OnItemCardClick);
        ClearCategoryCards();
    }

    void Setup()
    {
        TraderXUINavigationService.GetInstance().SetNavigationId(ENavigationIds.PURCHASE);
        TraderXTradingService.GetInstance().SetTradeMode(ETraderXTradeMode.BUY);
        ShowCategoryList();
    }

    void ShowCategoryList()
    {
        ClearCategoryCards();
        array<ref TraderXCategory> categories = TraderXTradingService.GetInstance().GetTraderCategories();
        foreach (TraderXCategory category : categories)
        {
            if (!category || (category && !category.isVisible))
                continue;

            CategoryCardComponent card = new CategoryCardComponent();
            card.InitComponent(null);
            card.SetCategoryCardData(category, EItemCardSize.LARGE, false, ETraderXCategoryType.NONE);
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

    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (w == toggleExpandBtn)
        {
            TraderXInventoryManager.PlayMenuSound(ETraderXSounds.SELECT);
            m_AllExpanded = !m_AllExpanded;
            foreach (CategoryCardComponent card : m_CategoryCards)
            {
                card.SetExpanded(!m_AllExpanded);
            }
            if (toggleExpandText)
            {
                if (m_AllExpanded)
                    toggleExpandText.SetText(Widget.TranslateString("#tpm_collapse_all"));
                else
                    toggleExpandText.SetText(Widget.TranslateString("#tpm_expand_all"));
            }
            return true;
        }
        return false;
    }

    override bool OnMouseButtonDown(Widget w, int x, int y, int button)
    {
        PlayerPreviewController.GetInstance().OnMouseButtonDown(w, x, y, button);
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

        GetTraderXLogger().LogDebug(string.Format("Transaction request sent for item %1 with price %2", itemSelected.item.className, itemSelected.GetPrice()));
    }
}
