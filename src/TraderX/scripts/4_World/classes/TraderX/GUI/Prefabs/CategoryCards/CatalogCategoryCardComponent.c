class CatalogCategoryCardComponent extends DZlrComponentBase
{
    private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("traderx-catalog-category-card", CatalogCategoryCardComponent);

    ref array<ref CatalogItemCardComponent> m_ItemCards = new array<ref CatalogItemCardComponent>();

    ImageWidget categoryIcon, croissantIcon, decroissantIcon;
    TextWidget itemCountText, categoryNameText;
    Widget categoryCardPanel, dropdownParentPanel, licensePanel, itemCardGrid;
    ButtonWidget categoryCardBtn, sortBtn, collapseBtn;

    bool isExpanded;
    int sorting = ETraderXCategorySort.MINTOMAX;
    int itemCardViewSize;
    int categoryType;
    string category_name, item_count;
    bool isBlocked = false;
    ref TraderXCategory category;

    override protected void RegisterMetadata()
    {
        m_Metadata = new ComponentMetadata();
        m_Metadata.selector = "traderx-catalog-category-card";
        m_Metadata.templatePath = "TraderX/datasets/gui/TraderXPrefab/CategoryCard/CatalogCategoryCardView.layout";
    }

    void SetCategoryCardData(TraderXCategory category, int itemCardViewSize, bool expand = false, int categoryType = ETraderXCategoryType.NONE)
    {
        this.category = category;
        this.itemCardViewSize = itemCardViewSize;
        this.categoryType = categoryType;

        category_name = category.categoryName;
        item_count = category.productIds.Count().ToString() + " items";

        if (categoryIcon)
        {
            categoryIcon.LoadImageFile(0, category.icon);
            if (category.icon == string.Empty)
                categoryIcon.Show(false);
        }

        if (!TraderXLicenseService.GetInstance().HasLicenses(category.licensesRequired))
        {
            if (licensePanel) licensePanel.Show(true);
            if (categoryCardPanel) categoryCardPanel.SetAlpha(0.7);
            isBlocked = true;

            // Show missing license names in the lock panel tooltip
            string missingNames = TraderXLicenseService.GetInstance().GetMissingLicenseNames(category.licensesRequired);
            if (missingNames != "")
            {
                TextWidget licenseMsgWidget = TextWidget.Cast(licensePanel.FindAnyWidget("licenseMsgText"));
                if (licenseMsgWidget)
                    licenseMsgWidget.SetText("Requires: " + missingNames);
            }
        }

        if (category.productIds.Count() == 0 && categoryCardPanel)
            categoryCardPanel.SetAlpha(0.7);

        isExpanded = expand;
        HandleCollapse();

        if (categoryNameText) categoryNameText.SetText(category_name);
        if (itemCountText) itemCountText.SetText(item_count);
    }

    void SetExpanded(bool expand)
    {
        if (category.GetProducts().Count() == 0 || isBlocked)
            return;
        if (isExpanded == expand)
            return;

        isExpanded = expand;
        if (dropdownParentPanel) dropdownParentPanel.Show(!isExpanded);
        if (itemCountText) itemCountText.Show(isExpanded);

        if (isExpanded)
            ClearItemCards();
        else
        {
            ShowItemList();
            Sort(sorting == ETraderXCategorySort.MINTOMAX);
        }
    }

    void HandleCollapse()
    {
        if (category.GetProducts().Count() == 0 || isBlocked)
            return;

        isExpanded = !isExpanded;
        if (dropdownParentPanel) dropdownParentPanel.Show(!isExpanded);
        if (itemCountText) itemCountText.Show(isExpanded);

        if (isExpanded)
            ClearItemCards();
        else
        {
            ShowItemList();
            Sort(sorting == ETraderXCategorySort.MINTOMAX);
        }
    }

    void ClearItemCards()
    {
        foreach (CatalogItemCardComponent card : m_ItemCards)
        {
            if (card && card.GetRoot())
                card.GetRoot().Unlink();
        }
        m_ItemCards.Clear();
    }

    void ShowItemList()
    {
        ClearItemCards();
        array<ref TraderXProduct> products = category.GetProducts();
        foreach (TraderXProduct item : products)
        {
            if (!item.CanBeSold())
                continue;

            CatalogItemCardComponent card = new CatalogItemCardComponent(item, categoryType);
            card.InitComponent(null);
            if (dropdownParentPanel)
            {
                dropdownParentPanel.AddChild(card.GetRoot());
                card.GetRoot().Show(true);
            }
            m_ItemCards.Insert(card);
        }
    }

    void FilterList(string searchKeyword)
    {
        ClearItemCards();
        string searchLower = searchKeyword;
        searchLower.ToLower();
        array<ref TraderXProduct> products = category.GetProducts();
        foreach (TraderXProduct item : products)
        {
            if (!item.CanBeSold())
                continue;

            string classNameLower = item.className;
            classNameLower.ToLower();
            string displayNameLower = item.GetDisplayName();
            displayNameLower.ToLower();

            if (searchLower == string.Empty || classNameLower.Contains(searchLower) || displayNameLower.Contains(searchLower))
            {
                CatalogItemCardComponent card = new CatalogItemCardComponent(item, categoryType);
                card.InitComponent(null);
                if (dropdownParentPanel)
                {
                    dropdownParentPanel.AddChild(card.GetRoot());
                    card.GetRoot().Show(true);
                }
                m_ItemCards.Insert(card);
            }
        }
    }

    void Sort(bool ascending)
    {
        bool isSwapped = true;
        int n = m_ItemCards.Count();
        while (isSwapped)
        {
            isSwapped = false;
            for (int i = 1; i < n; i++)
            {
                int aP = m_ItemCards[i - 1].GetPrice();
                int bP = m_ItemCards[i].GetPrice();
                if ((ascending && aP > bP) || (!ascending && aP < bP))
                {
                    m_ItemCards.SwapItems(i - 1, i);
                    isSwapped = true;
                }
            }
            n--;
        }
        if (dropdownParentPanel)
        {
            foreach (CatalogItemCardComponent card : m_ItemCards)
            {
                if (card && card.GetRoot())
                {
                    card.GetRoot().Unlink();
                    dropdownParentPanel.AddChild(card.GetRoot());
                }
            }
        }
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (w == categoryCardBtn)
        {
            TraderXInventoryManager.PlayMenuSound(ETraderXSounds.QUICK_EVENT);
            HandleCollapse();
            return true;
        }
        if (w == sortBtn)
        {
            TraderXInventoryManager.PlayMenuSound(ETraderXSounds.SELECT);
            if (sorting == ETraderXCategorySort.MINTOMAX)
            {
                sorting = ETraderXCategorySort.MAXTOMIN;
                if (croissantIcon) croissantIcon.Show(true);
                if (decroissantIcon) decroissantIcon.Show(false);
            }
            else
            {
                sorting = ETraderXCategorySort.MINTOMAX;
                if (croissantIcon) croissantIcon.Show(false);
                if (decroissantIcon) decroissantIcon.Show(true);
            }
            Sort(sorting == ETraderXCategorySort.MINTOMAX);
            return true;
        }
        if (w == collapseBtn)
        {
            TraderXInventoryManager.PlayMenuSound(ETraderXSounds.SELECT);
            HandleCollapse();
            return true;
        }
        return false;
    }
}
