class CategoryCardComponent extends DZlrComponentBase
{
    private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("traderx-category-card", CategoryCardComponent);

    ref array<ref ItemCardComponent> m_ItemCards = new array<ref ItemCardComponent>();

    ImageWidget categoryIcon, croissantIcon, decroissantIcon;
    TextWidget itemCountText, categoryNameText;
    Widget categoryCardPanel, dropdownParentPanel, licensePanel, itemCardGrid;
    ButtonWidget categoryCardBtn, sortBtn, collapseBtn;

    bool isExpanded;
    int sorting = ETraderXCategorySort.MINTOMAX;
    int itemCardViewSize;
    int categoryType;
    string category_name;
    string item_count;
    bool isBlocked = false;
    ref TraderXCategory category;

    override protected void RegisterMetadata()
    {
        m_Metadata = new ComponentMetadata();
        m_Metadata.selector = "traderx-category-card";
        m_Metadata.templatePath = "TraderX/datasets/gui/TraderXPrefab/CategoryCard/CategoryCardLargeView.layout";
    }

    array<ref ItemCardComponent> GetItemCards()
    {
        return m_ItemCards;
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

        if (category.productIds.Count() == 0)
        {
            if (categoryCardPanel) categoryCardPanel.SetAlpha(0.7);
        }

        isExpanded = expand;
        HandleCollapse();

        if (categoryNameText) categoryNameText.SetText(category_name);
        if (itemCountText) itemCountText.SetText(item_count);
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

    void ClearItemCards()
    {
        foreach (ItemCardComponent card : m_ItemCards)
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
            if (!item.CanBeBought())
                continue;

            ItemCardComponent card = ItemCardComponent.CreateItemCard(item, itemCardViewSize, categoryType);
            if (card && dropdownParentPanel)
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
            if (!item.CanBeBought())
                continue;

            string classNameLower = item.className;
            classNameLower.ToLower();
            string displayNameLower = item.GetDisplayName();
            displayNameLower.ToLower();
            if (classNameLower.Contains(searchLower) || displayNameLower.Contains(searchLower))
            {
                ItemCardComponent card = ItemCardComponent.CreateItemCard(item, itemCardViewSize, categoryType);
                if (card && dropdownParentPanel)
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
                ItemCardComponent a = m_ItemCards[i - 1];
                ItemCardComponent b = m_ItemCards[i];

                int aProperty = a.GetPrice();
                int bProperty = b.GetPrice();

                if (ascending && aProperty > bProperty)
                {
                    m_ItemCards.SwapItems(i - 1, i);
                    isSwapped = true;
                }
                else if (!ascending && aProperty < bProperty)
                {
                    m_ItemCards.SwapItems(i - 1, i);
                    isSwapped = true;
                }
            }
            n--;
        }

        // Reorder widgets
        if (dropdownParentPanel)
        {
            foreach (ItemCardComponent card : m_ItemCards)
            {
                if (card && card.GetRoot())
                {
                    card.GetRoot().Unlink();
                    dropdownParentPanel.AddChild(card.GetRoot());
                }
            }
        }
    }
}
