class CategoryCardViewController: ViewController
{
    ref ObservableCollection<ref ItemCardView> item_card_list = new ObservableCollection<ref ItemCardView>(this);
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
            item_card_list.Clear();
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
        int n = item_card_list.GetArray().Count();

        while(isSwapped)
        {
            isSwapped = false;
            for (int i = 1; i < n; i++)
            {
                ItemCardView a = item_card_list.Get(i - 1);
                ItemCardView b = item_card_list.Get(i);

                int aProperty = a.GetTemplateController().GetPrice();
                int bProperty = b.GetTemplateController().GetPrice();

                if(ascending && aProperty > bProperty)
                {
                    item_card_list.SwapItems(i - 1, i);
                    isSwapped = true;
                }else if (aProperty < bProperty){
                    item_card_list.SwapItems(i - 1, i);
                    isSwapped = true;
                }
            }
            n--;
        }
    }

    void FilterList(string searchKeyword)
    {
        item_card_list.Clear();
        array<ref TraderXProduct> products = category.GetProducts();
        foreach(TraderXProduct item : products)
        {
            if(!item.CanBeBought())
                continue;
                
            if(item.className.Contains(searchKeyword) || item.GetDisplayName().Contains(searchKeyword)){
                item_card_list.Insert(ItemCardView.CreateItemCardView(item, itemCardViewSize, categoryType));
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
        item_card_list.Clear();
        array<ref TraderXProduct> products = category.GetProducts();
        foreach(TraderXProduct item : products)
        {
            if(!item.CanBeBought())
                continue;
                
            item_card_list.Insert(ItemCardView.CreateItemCardView(item, itemCardViewSize, categoryType));
        }
    }
}