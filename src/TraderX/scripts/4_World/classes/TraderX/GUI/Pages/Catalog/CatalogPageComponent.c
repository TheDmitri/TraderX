class CatalogPageComponent extends DZlrComponentBase
{
    private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("traderx-catalog-page", CatalogPageComponent);

    string search_keyword;
    ref array<ref CatalogCategoryCardComponent> m_CategoryCards = new array<ref CatalogCategoryCardComponent>();
    bool m_AllExpanded = false;

    // Selected item details
    string selected_item_name;
    string selected_sell_price;
    string selected_buy_price;
    string selected_stock;
    EntityAI selected_preview;
    string selected_description;
    ref TraderXProduct selected_item;

    // Widgets
    Widget category_card_list;
    TextWidget toggleExpandText, selectedItemNameText, selectedSellPriceText, selectedBuyPriceText, selectedStockText, selectedDescriptionText;
    ButtonWidget toggleExpandBtn;
    EditBoxWidget searchBox;
    ItemPreviewWidget selectedPreviewWidget;

    override protected void RegisterMetadata()
    {
        m_Metadata = new ComponentMetadata();
        m_Metadata.selector = "traderx-catalog-page";
        m_Metadata.templatePath = "TraderX/datasets/gui/CatalogPage/CatalogPageView.layout";
    }

    override void OnInit()
    {
        CatalogItemCardComponent.Event_OnCatalogItemCardClickCallBack.Insert(OnCatalogItemCardClick);
        Setup();
    }

    override void OnDestroy()
    {
        CatalogItemCardComponent.Event_OnCatalogItemCardClickCallBack.Remove(OnCatalogItemCardClick);
        ClearCategoryCards();
    }

    void Setup()
    {
        TraderXUINavigationService.GetInstance().SetNavigationId(ENavigationIds.CATALOG);
        TraderXTradingService.GetInstance().SetTradeMode(ETraderXTradeMode.SELL);
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

            if (!HasSellableProducts(category))
                continue;

            CatalogCategoryCardComponent card = new CatalogCategoryCardComponent();
            card.InitComponent(null);
            card.SetCategoryCardData(category, EItemCardSize.LARGE, false, ETraderXCategoryType.NONE);
            AddCategoryCard(card);
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

    void ClearCategoryCards()
    {
        foreach (CatalogCategoryCardComponent card : m_CategoryCards)
        {
            if (card && card.GetRoot())
                card.GetRoot().Unlink();
        }
        m_CategoryCards.Clear();
    }

    void AddCategoryCard(CatalogCategoryCardComponent card)
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
        foreach (CatalogCategoryCardComponent categoryCard : m_CategoryCards)
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
            foreach (CatalogCategoryCardComponent card : m_CategoryCards)
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

    void OnCatalogItemCardClick(CatalogItemCardComponent itemSelected, int command = ETraderXClickEvents.LCLICK)
    {
        switch (command)
        {
            case ETraderXClickEvents.LCLICK:
                UpdateSelectedItemDetails(itemSelected);
                break;
            case ETraderXClickEvents.RCLICK:
                OnCatalogItemCardRClick(itemSelected);
                break;
        }
    }

    void UpdateSelectedItemDetails(CatalogItemCardComponent itemSelected)
    {
        selected_item = itemSelected.GetItem();
        selected_item_name = selected_item.GetDisplayName();
        selected_preview = itemSelected.GetPreview();

        int sellPrice = TraderXPricingService.GetInstance().GetPricePreview(selected_item.GetProductId(), false, 1, TraderXItemState.PRISTINE);
        selected_sell_price = sellPrice.ToString();

        int buyPrice = TraderXPricingService.GetInstance().GetPricePreview(selected_item.GetProductId(), true, 1, TraderXItemState.PRISTINE);
        if (selected_item.defaultPreset)
        {
            int presetPrice = TraderXPresetsService.GetInstance().CalculateTotalPricePreset(selected_item.defaultPreset);
            buyPrice = presetPrice;
        }
        selected_buy_price = buyPrice.ToString();

        selected_stock = TraderXProductStockRepository.GetStockDisplay(selected_item.productId, selected_item.maxStock);

        selected_description = itemSelected.GetItemDescription();
        if (selected_description == string.Empty)
            selected_description = "#tpm_no_description_available";

        // Update widgets
        if (selectedItemNameText) selectedItemNameText.SetText(selected_item_name);
        if (selectedSellPriceText) selectedSellPriceText.SetText(selected_sell_price);
        if (selectedBuyPriceText) selectedBuyPriceText.SetText(selected_buy_price);
        if (selectedStockText) selectedStockText.SetText(selected_stock);
        if (selectedDescriptionText) selectedDescriptionText.SetText(selected_description);
        if (selectedPreviewWidget) selectedPreviewWidget.SetItem(selected_preview);
    }

    void OnCatalogItemCardRClick(CatalogItemCardComponent itemSelected)
    {
        if (!itemSelected.GetItem().HasVariants() && !itemSelected.GetItem().HasAttachments())
            return;

        CustomizePageComponent customizePage = new CustomizePageComponent(itemSelected.GetItem());
        customizePage.InitComponent(null);
        TraderXMainComponent.GetInstance().AddSubPage(customizePage);
    }
}
