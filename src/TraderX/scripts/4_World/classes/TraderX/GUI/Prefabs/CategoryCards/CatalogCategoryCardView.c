class CatalogCategoryCardViewController: ViewController
{
    ref ObservableCollection<ref CatalogItemCardView> catalog_item_card_list = new ObservableCollection<ref CatalogItemCardView>(this);
    ImageWidget categoryIcon;
    ImageWidget croissantIcon;
    ImageWidget decroissantIcon;

    TextWidget itemCountText;

    Widget categoryCardPanel, dropdownParentPanel, licensePanel, itemCardGrid;

    bool isExpanded;
    int sorting = ETraderXCategorySort.MINTOMAX;
    int itemCardViewSize;
    int categoryType;

    string category_name;
    string item_count;

    bool isBlocked = false;

    ref TraderXCategory category;
    string searchKeyword;

    void SetCategoryCardData(TraderXCategory category, int itemCardViewSize, bool expand = false, int categoryType = ETraderXCategoryType.NONE)
    {
        GetTraderXLogger().LogDebug("SetCategoryCardData " + category.categoryName + " expand: " + expand); 
        this.category = category;
        this.itemCardViewSize = itemCardViewSize;
        this.categoryType = categoryType;

        category_name = category.categoryName;
        item_count = category.productIds.Count().ToString() + " items";

        categoryIcon.LoadImageFile(0, category.icon);
        if(category.icon == string.Empty){
            categoryIcon.Show(false);
        }

        if(!TraderXLicenseService.GetInstance().HasLicenses(category.licensesRequired)){
            licensePanel.Show(true);
            if(categoryCardPanel)
            categoryCardPanel.SetAlpha(0.7);
            isBlocked = true;
        }

        if(category.productIds.Count() == 0){
            if(categoryCardPanel)
                categoryCardPanel.SetAlpha(0.7);
        }

        isExpanded = expand;
        HandleCollapse();

        NotifyPropertiesChanged({"category_name", "item_count"});
    }

    bool OnCategoryCardExecute(ButtonCommandArgs args)
    {
        TraderXInventoryManager.PlayMenuSound(ETraderXSounds.QUICK_EVENT);
        HandleCollapse();
        return true;
    }

    void HandleCollapse()
    {
        if(category.GetProducts().Count() == 0 || isBlocked)
            return;

        isExpanded = !isExpanded;

        dropdownParentPanel.Show(!isExpanded);
        itemCountText.Show(isExpanded);

        if (isExpanded){
            catalog_item_card_list.Clear();
        }
        else{
            ShowItemList();
            Sort(sorting == ETraderXCategorySort.MINTOMAX);
        }
    }

    void SetExpanded(bool expand)
    {
        if(category.GetProducts().Count() == 0 || isBlocked)
            return;

        if(isExpanded == expand)
            return;

        isExpanded = expand;
        dropdownParentPanel.Show(!isExpanded);
        itemCountText.Show(isExpanded);

        if (isExpanded){
            catalog_item_card_list.Clear();
        }
        else{
            ShowItemList();
            Sort(sorting == ETraderXCategorySort.MINTOMAX);
        }
    }

    bool OnSortByClic(ButtonCommandArgs args)
    {
        TraderXInventoryManager.PlayMenuSound(ETraderXSounds.SELECT);
        if(sorting == ETraderXCategorySort.MINTOMAX){
            sorting = ETraderXCategorySort.MAXTOMIN;
            croissantIcon.Show(true);
            decroissantIcon.Show(false);
        }
        else if(sorting == ETraderXCategorySort.MAXTOMIN){
            sorting = ETraderXCategorySort.MINTOMAX;
            croissantIcon.Show(false);
            decroissantIcon.Show(true);
        }

        Sort(sorting == ETraderXCategorySort.MINTOMAX);
        return true;
    }

    void Sort(bool ascending)
    {
        bool isSwapped = true;
        int n = catalog_item_card_list.GetArray().Count();

        while(isSwapped)
        {
            isSwapped = false;
            for (int i = 1; i < n; i++)
            {
                CatalogItemCardView a = catalog_item_card_list.Get(i - 1);
                CatalogItemCardView b = catalog_item_card_list.Get(i);

                int aProperty = a.GetTemplateController().GetPrice();
                int bProperty = b.GetTemplateController().GetPrice();

                if(ascending && aProperty > bProperty)
                {
                    catalog_item_card_list.SwapItems(i - 1, i);
                    isSwapped = true;
                }else if (aProperty < bProperty){
                    catalog_item_card_list.SwapItems(i - 1, i);
                    isSwapped = true;
                }
            }
            n--;
        }
    }

    void FilterList(string searchKeyword)
    {
        this.searchKeyword = searchKeyword;
        catalog_item_card_list.Clear();
        string searchLower = searchKeyword;
        searchLower.ToLower();
        array<ref TraderXProduct> products = category.GetProducts();
        foreach(TraderXProduct item : products)
        {
            if(!item.CanBeSold())
                continue;
            
            string classNameLower = item.className;
            classNameLower.ToLower();
            string displayNameLower = item.GetDisplayName();
            displayNameLower.ToLower();
            if(searchLower == string.Empty || classNameLower.Contains(searchLower) || displayNameLower.Contains(searchLower)){
                catalog_item_card_list.Insert(new CatalogItemCardView(item, categoryType));
            }
        }
    }

    bool OnCollapseClic(ButtonCommandArgs args)
    {
        TraderXInventoryManager.PlayMenuSound(ETraderXSounds.SELECT);
        HandleCollapse();
        return true;
    }

    void ShowItemList()
    {
        catalog_item_card_list.Clear();
        array<ref TraderXProduct> products = category.GetProducts();
        foreach(TraderXProduct item : products)
        {
            if(!item.CanBeSold())
                continue;
                
            catalog_item_card_list.Insert(new CatalogItemCardView(item, categoryType));
        }
    }
}

class CatalogCategoryCardView: ScriptViewTemplate<CatalogCategoryCardViewController>
{
    void CatalogCategoryCardView(TraderXCategory category, int itemCardViewSize, bool expand = false, int categoryType = ETraderXCategoryType.NONE)
    {
        m_TemplateController.SetCategoryCardData(category, itemCardViewSize, expand, categoryType);
    }

    override string GetLayoutFile() 
    {
        return "TraderX/datasets/gui/TraderXPrefab/CategoryCard/CatalogCategoryCardView.layout";
    }

    override typename GetControllerType()
    {
        return CatalogCategoryCardViewController;
    }
}
