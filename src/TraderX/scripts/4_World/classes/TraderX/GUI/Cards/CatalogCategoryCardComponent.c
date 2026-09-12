class CatalogCategoryCardComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("tx-catalog-category-card", CatalogCategoryCardComponent);

	// Fired when a category collapses so the page can snap its scroll back to the top.
	static ref ScriptInvoker Event_OnCategoryCollapsedCallBack = new ScriptInvoker();

	ref TraderXCategory _inputCategory;
	int itemCardViewSize;
	int categoryType;

	string category_name;
	string item_count;

	bool isExpanded;
	bool isBlocked = false;
	int sorting = ETraderXCategorySort.MINTOMAX;
	string searchKeyword;

	ref array<ref TraderXProduct> _inputCatalogProducts = new array<ref TraderXProduct>();

	ImageWidget categoryIcon;
	ImageWidget croissantIcon;
	ImageWidget decroissantIcon;
	TextWidget itemCountText;
	Widget categoryCardPanel;
	Widget dropdownParentPanel;
	Widget licensePanel;

	// List/grid view containers (toggled by TraderXViewModeService).
	Widget itemCardGridList;
	Widget itemCardGridTiles;

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "tx-catalog-category-card";
		m_Metadata.templatePath = "TraderX/datasets/gui/TraderXPrefab/CategoryCard/CatalogCategoryCardView.layout";
	}

	override void OnInit()
	{
		AutoLoadWidgets();
		TraderXViewModeService.Event_OnViewModeChangedCallBack.Insert(OnViewModeChanged);
		if (_inputCategory)
			SetCategoryCardData(_inputCategory, 0);
		TraderXViewModeBinder.Apply(itemCardGridList, itemCardGridTiles);
	}

	override void OnDestroy()
	{
		TraderXViewModeService.Event_OnViewModeChangedCallBack.Remove(OnViewModeChanged);
	}

	void OnViewModeChanged(bool grid)
	{
		TraderXViewModeBinder.Apply(itemCardGridList, itemCardGridTiles);
	}

	void SetCategoryCardData(TraderXCategory category, int cardSize, bool expand = false, int catType = ETraderXCategoryType.NONE)
	{
		_inputCategory = category;
		itemCardViewSize = cardSize;
		categoryType = catType;

		category_name = category.categoryName;
		item_count = category.productIds.Count().ToString() + " items";

		if (categoryIcon)
		{
			// category.icon is only usable as an image when it's an actual texture path;
			// it's frequently an emoji (📦/💚/⚙️) or empty, which LoadImageFile can't load
			// (→ blank/orange square). Fall back to a default glyph in those cases. Always
			// LoadImageFile (not just Show) so a recycled card doesn't keep the previous icon.
			if (TraderXCategoryIconUtils.IsIconFilePath(category.icon))
				categoryIcon.LoadImageFile(0, category.icon);
			// else: keep the layout's built-in imageset default glyph. category.icon here is an
			// emoji or empty (never a texture), so loading it would blank the widget.
			categoryIcon.Show(true);
		}

		if (!TraderXLicenseService.GetInstance().HasLicenses(category.licensesRequired))
		{
			if (licensePanel) licensePanel.Show(true);
			if (categoryCardPanel) categoryCardPanel.SetAlpha(0.7);
			isBlocked = true;
		}

		if (category.productIds.Count() == 0)
		{
			if (categoryCardPanel) categoryCardPanel.SetAlpha(0.7);
		}

		isExpanded = expand;
		HandleCollapse();
		MarkForCheck();
	}

	void OnCategoryCardClick()
	{
		TraderXInventoryManager.PlayMenuSound(ETraderXSounds.QUICK_EVENT);
		HandleCollapse();
	}

	void HandleCollapse()
	{
		if (_inputCategory.GetProducts().Count() == 0 || isBlocked) return;
		isExpanded = !isExpanded;
		if (dropdownParentPanel) dropdownParentPanel.Show(!isExpanded);
		if (itemCountText) itemCountText.Show(isExpanded);
		if (isExpanded)
		{
			_inputCatalogProducts.Clear();
			Event_OnCategoryCollapsedCallBack.Invoke();
		}
		else
		{
			ShowItemList();
			SortProducts(sorting == ETraderXCategorySort.MINTOMAX);
		}
		MarkForCheck();
		TraderXViewModeBinder.Apply(itemCardGridList, itemCardGridTiles);
	}

	void SetExpanded(bool expand)
	{
		if (_inputCategory.GetProducts().Count() == 0 || isBlocked) return;
		if (isExpanded == expand) return;
		isExpanded = expand;
		if (dropdownParentPanel) dropdownParentPanel.Show(!isExpanded);
		if (itemCountText) itemCountText.Show(isExpanded);
		if (isExpanded)
			_inputCatalogProducts.Clear();
		else
		{
			ShowItemList();
			SortProducts(sorting == ETraderXCategorySort.MINTOMAX);
		}
		MarkForCheck();
		TraderXViewModeBinder.Apply(itemCardGridList, itemCardGridTiles);
	}

	void OnSortClick()
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
		SortProducts(sorting == ETraderXCategorySort.MINTOMAX);
		// Re-emit the list so the foreach re-renders rows in the new order — an in-place
		// reorder of the same array doesn't trigger a rebuild on its own.
		array<ref TraderXProduct> sortedCopy = new array<ref TraderXProduct>();
		foreach (TraderXProduct sp : _inputCatalogProducts)
			sortedCopy.Insert(sp);
		_inputCatalogProducts.Clear();
		foreach (TraderXProduct sp2 : sortedCopy)
			_inputCatalogProducts.Insert(sp2);
		MarkForCheck();
	}

	void SortProducts(bool ascending)
	{
		bool isSwapped = true;
		int n = _inputCatalogProducts.Count();
		while (isSwapped)
		{
			isSwapped = false;
			for (int i = 1; i < n; i++)
			{
				int aPrice = TraderXPricingService.GetInstance().GetPricePreview(_inputCatalogProducts[i - 1].GetProductId(), false, 1, TraderXItemState.PRISTINE);
				int bPrice = TraderXPricingService.GetInstance().GetPricePreview(_inputCatalogProducts[i].GetProductId(), false, 1, TraderXItemState.PRISTINE);
				if (ascending && aPrice > bPrice)
				{
					_inputCatalogProducts.SwapItems(i - 1, i);
					isSwapped = true;
				}
				else if (!ascending && aPrice < bPrice)
				{
					_inputCatalogProducts.SwapItems(i - 1, i);
					isSwapped = true;
				}
			}
			n--;
		}
	}

	void FilterList(string keyword)
	{
		searchKeyword = keyword;
		_inputCatalogProducts.Clear();
		string searchLower = keyword;
		searchLower.ToLower();
		array<ref TraderXProduct> products = _inputCategory.GetProducts();
		foreach (TraderXProduct item : products)
		{
			if (!item.CanBeSold()) continue;
			string classNameLower = item.className;
			classNameLower.ToLower();
			string displayNameLower = item.GetDisplayName();
			displayNameLower.ToLower();
			if (searchLower == string.Empty || classNameLower.Contains(searchLower) || displayNameLower.Contains(searchLower))
				_inputCatalogProducts.Insert(item);
		}
		MarkForCheck();
	}

	// Called by the page on search-keyword change. Empty keyword collapses; a keyword
	// expands the category to its filtered matches (or collapses if there are none).
	void OnSearch(string keyword)
	{
		if (isBlocked) return;

		if (keyword == "")
		{
			isExpanded = true;
			if (dropdownParentPanel) dropdownParentPanel.Show(false);
			if (itemCountText) itemCountText.Show(true);
			_inputCatalogProducts.Clear();
			MarkForCheck();
			TraderXViewModeBinder.Apply(itemCardGridList, itemCardGridTiles);
			return;
		}

		FilterList(keyword);
		bool hasMatches = _inputCatalogProducts.Count() > 0;
		isExpanded = !hasMatches;
		if (dropdownParentPanel) dropdownParentPanel.Show(hasMatches);
		if (itemCountText) itemCountText.Show(!hasMatches);
		MarkForCheck();
		TraderXViewModeBinder.Apply(itemCardGridList, itemCardGridTiles);
	}

	void OnCollapseClick()
	{
		TraderXInventoryManager.PlayMenuSound(ETraderXSounds.SELECT);
		HandleCollapse();
	}

	void ShowItemList()
	{
		_inputCatalogProducts.Clear();
		array<ref TraderXProduct> products = _inputCategory.GetProducts();
		foreach (TraderXProduct item : products)
		{
			if (!item.CanBeSold()) continue;
			_inputCatalogProducts.Insert(item);
		}
	}
}
