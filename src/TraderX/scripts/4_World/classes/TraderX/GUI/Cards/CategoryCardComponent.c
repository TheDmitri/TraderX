class CategoryCardComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("tx-category-card", CategoryCardComponent);

	// Fired when a category collapses so the page can snap its scroll back to the top.
	static ref ScriptInvoker Event_OnCategoryCollapsedCallBack = new ScriptInvoker();

	ref TraderXCategory _inputCategory;
	// Category the card was last filled from (see OnChanges).
	private TraderXCategory m_BoundCategory;
	int itemCardViewSize;
	int categoryType;

	string category_name;
	string item_count;
	// "Licenses Required: Car Licence" — bound into the licensePanel badge.
	string license_text;

	bool isExpanded;
	// Purchase / favorites pages: the section never collapses (no accordion).
	bool m_AlwaysExpanded;
	// Licence-locked: the card stays browsable but is greyed (licenseDimPanel) and shows the
	// licensePanel badge. Buying is refused by the item card / checkout and by the server.
	bool isBlocked = false;
	int sorting = ETraderXCategorySort.MINTOMAX;

	ref array<ref TraderXProduct> _inputItemProducts = new array<ref TraderXProduct>();

	ImageWidget categoryIcon;
	ImageWidget favStarIcon;
	ImageWidget croissantIcon;
	ImageWidget decroissantIcon;
	TextWidget itemCountText;
	Widget categoryCardPanel;
	Widget dropdownParentPanel;
	Widget licensePanel;
	Widget licenseDimPanel;

	// List/grid view containers (toggled by TraderXViewModeService).
	Widget itemCardGridList;
	Widget itemCardGridTiles;

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "tx-category-card";
		m_Metadata.templatePath = "TraderX/datasets/gui/TraderXPrefab/CategoryCard/CategoryCardLargeView.layout";
	}

	override void OnInit()
	{
		AutoLoadWidgets();
		TraderXViewModeService.Event_OnViewModeChangedCallBack.Insert(OnViewModeChanged);
		TraderXLicenseService.Event_OnPlayerLicensesChanged.Insert(OnPlayerLicensesChanged);
		if (_inputCategory)
			SetCategoryCardData(_inputCategory, 0);
		TraderXViewModeBinder.Apply(itemCardGridList, itemCardGridTiles);
	}

	override void OnDestroy()
	{
		TraderXViewModeService.Event_OnViewModeChangedCallBack.Remove(OnViewModeChanged);
		TraderXLicenseService.Event_OnPlayerLicensesChanged.Remove(OnPlayerLicensesChanged);
	}

	// Licence bought (or licences synced late): un-grey without reopening the menu.
	void OnPlayerLicensesChanged()
	{
		if (!_inputCategory) return;
		ApplyLicenseState();
		MarkForCheck();
	}

	// Greyed-but-browsable state for categories whose licensesRequired the player lacks.
	void ApplyLicenseState()
	{
		isBlocked = !TraderXLicenseService.GetInstance().HasLicenses(_inputCategory.licensesRequired);
		license_text = string.Empty;
		if (isBlocked)
			license_text = TraderXLicenseService.FormatRequiredLabel(_inputCategory.licensesRequired);

		if (licensePanel) licensePanel.Show(isBlocked);
		if (licenseDimPanel) licenseDimPanel.Show(isBlocked);
		if (categoryCardPanel)
			categoryCardPanel.SetAlpha(DZTernaryFloat.If(isBlocked || GetDisplayableProductCount() == 0, 0.7, 1.0));
	}

	void OnViewModeChanged(bool grid)
	{
		TraderXViewModeBinder.Apply(itemCardGridList, itemCardGridTiles);
	}

	// DayZular's ForEach reuses this card when the list keeps its length and only calls
	// OnChanges with the new item (the Favorites page rebuilds its single category on every
	// star toggle). Re-read the category then, keeping the open/closed state.
	override void OnChanges()
	{
		if (!_inputCategory || _inputCategory == m_BoundCategory)
			return;

		bool wasCollapsed = isExpanded; // isExpanded == true means collapsed (see HandleCollapse)
		SetCategoryCardData(_inputCategory, itemCardViewSize, !wasCollapsed, categoryType);

		// HandleCollapse ignores empty categories, so drop the old item cards here.
		if (GetDisplayableProductCount() == 0)
		{
			isExpanded = true;
			_inputItemProducts.Clear();
			if (dropdownParentPanel) dropdownParentPanel.Show(false);
			if (itemCountText) itemCountText.Show(true);
			MarkForCheck();
		}
	}

	void SetCategoryCardData(TraderXCategory category, int cardSize, bool expand = false, int catType = ETraderXCategoryType.NONE)
	{
		_inputCategory = category;
		m_BoundCategory = category;
		itemCardViewSize = cardSize;
		categoryType = catType;

		// translated here so the layout's uppercase pipe works on text, not on a #key
		category_name = Widget.TranslateString(category.categoryName);
		item_count = GetDisplayableProductCount().ToString() + " items";

		// The favorites pseudo-category has no icon file — show the star glyph instead.
		bool isFavoritesCategory = category.categoryName == "#tpm_favorites";
		if (favStarIcon)
			favStarIcon.Show(isFavoritesCategory);

		if (categoryIcon)
		{
			if (isFavoritesCategory)
			{
				categoryIcon.Show(false);
			}
			else
			{
				// category.icon is only usable as an image when it's a real texture path;
				// it's frequently an emoji (📦/💚/⚙️) or empty, which LoadImageFile can't load.
				// Fall back to a default glyph then. Always LoadImageFile (not just Show) so a
				// recycled card doesn't keep the previous category's icon.
				if (TraderXCategoryIconUtils.IsIconFilePath(category.icon))
					categoryIcon.LoadImageFile(0, category.icon);
				// else: keep the layout's built-in imageset default glyph. category.icon here is
				// an emoji or empty (never a texture), so loading it would blank the widget.
				categoryIcon.Show(true);
			}
		}

		ApplyLicenseState();

		isExpanded = expand;
		HandleCollapse();
		MarkForCheck();
	}

	// Products this card will actually list when expanded — buy pages filter CanBeBought.
	// The label/count must match what the expand reveals or a sell-only category shows
	// "10 items" then opens an empty list (reads like "products don't load").
	int GetDisplayableProductCount()
	{
		int count = 0;
		array<ref TraderXProduct> products = _inputCategory.GetProducts();
		foreach (TraderXProduct item : products)
		{
			if (item && item.CanBeBought())
				count++;
		}
		return count;
	}

	void OnCategoryCardClick()
	{
		TraderXInventoryManager.PlayMenuSound(ETraderXSounds.QUICK_EVENT);
		HandleCollapse();
	}

	// Page-level section: expand now and ignore collapse clicks from then on.
	void ExpandAlways()
	{
		m_AlwaysExpanded = true;
		SetExpanded(false);
		if (itemCountText) itemCountText.Show(true);
	}

	void HandleCollapse()
	{
		if (GetDisplayableProductCount() == 0) return;
		if (m_AlwaysExpanded && !isExpanded) return;

		isExpanded = !isExpanded;
		if (dropdownParentPanel) dropdownParentPanel.Show(!isExpanded);
		if (itemCountText) itemCountText.Show(isExpanded || m_AlwaysExpanded);

		if (isExpanded)
		{
			_inputItemProducts.Clear();
			// isExpanded == true means collapsed here — tell the page to scroll to top.
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
		if (GetDisplayableProductCount() == 0) return;
		if (isExpanded == expand) return;
		isExpanded = expand;
		if (dropdownParentPanel) dropdownParentPanel.Show(!isExpanded);
		if (itemCountText) itemCountText.Show(isExpanded || m_AlwaysExpanded);
		if (isExpanded)
			_inputItemProducts.Clear();
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
		// Re-emit the list so the foreach actually re-renders the rows in the new order —
		// an in-place reorder of the same array doesn't trigger a rebuild on its own.
		array<ref TraderXProduct> sortedCopy = new array<ref TraderXProduct>();
		foreach (TraderXProduct sp : _inputItemProducts)
			sortedCopy.Insert(sp);
		_inputItemProducts.Clear();
		foreach (TraderXProduct sp2 : sortedCopy)
			_inputItemProducts.Insert(sp2);
		MarkForCheck();
	}

	void SortProducts(bool ascending)
	{
		int n = _inputItemProducts.Count();
		if (n < 2) return;

		// Compute each item's price ONCE up front. The old bubble sort called the
		// (heavy) GetPricePreview twice per comparison — O(n^2) price calcs per
		// category, which made expand-all crawl. Now it's O(n) calcs + a swap-only sort.
		array<int> prices = new array<int>();
		for (int p = 0; p < n; p++)
			prices.Insert(TraderXPricingService.GetInstance().GetPricePreview(_inputItemProducts[p].GetProductId(), true, 1, TraderXItemState.PRISTINE));

		bool isSwapped = true;
		while (isSwapped)
		{
			isSwapped = false;
			for (int i = 1; i < n; i++)
			{
				if ((ascending && prices[i - 1] > prices[i]) || (!ascending && prices[i - 1] < prices[i]))
				{
					_inputItemProducts.SwapItems(i - 1, i);
					prices.SwapItems(i - 1, i);
					isSwapped = true;
				}
			}
			n--;
		}
	}

	void FilterList(string searchKeyword)
	{
		_inputItemProducts.Clear();
		string searchLower = searchKeyword;
		searchLower.ToLower();
		array<ref TraderXProduct> products = _inputCategory.GetProducts();
		foreach (TraderXProduct item : products)
		{
			if (!item.CanBeBought()) continue;
			string classNameLower = item.className;
			classNameLower.ToLower();
			string displayNameLower = item.GetDisplayName();
			displayNameLower.ToLower();
			if (classNameLower.Contains(searchLower) || displayNameLower.Contains(searchLower))
				_inputItemProducts.Insert(item);
		}
		MarkForCheck();
	}

	// Called by the page on search-keyword change. Empty keyword collapses; a keyword
	// expands the category to its filtered matches (or collapses if there are none).
	void OnSearch(string keyword)
	{
		// page section: an empty search shows the whole category again, a keyword filters it
		if (m_AlwaysExpanded)
		{
			if (keyword == "")
			{
				isExpanded = true;
				SetExpanded(false);
			}
			else
			{
				FilterList(keyword);
				TraderXViewModeBinder.Apply(itemCardGridList, itemCardGridTiles);
			}
			if (itemCountText) itemCountText.Show(true);
			return;
		}
		if (keyword == "")
		{
			isExpanded = true;
			if (dropdownParentPanel) dropdownParentPanel.Show(false);
			if (itemCountText) itemCountText.Show(true);
			_inputItemProducts.Clear();
			MarkForCheck();
			TraderXViewModeBinder.Apply(itemCardGridList, itemCardGridTiles);
			return;
		}

		FilterList(keyword);
		bool hasMatches = _inputItemProducts.Count() > 0;
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
		_inputItemProducts.Clear();
		array<ref TraderXProduct> products = _inputCategory.GetProducts();
		foreach (TraderXProduct item : products)
		{
			if (!item.CanBeBought()) continue;
			_inputItemProducts.Insert(item);
		}
	}
}

class CategoryCardMediumComponent extends CategoryCardComponent
{
	private static ref ComponentAutoRegister s_RegisterMedium = new ComponentAutoRegister("tx-category-card-medium", CategoryCardMediumComponent);

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "tx-category-card-medium";
		m_Metadata.templatePath = "TraderX/datasets/gui/TraderXPrefab/CategoryCard/CategoryCardMediumView.layout";
	}
}

class CategoryCardSmallComponent extends CategoryCardComponent
{
	private static ref ComponentAutoRegister s_RegisterSmall = new ComponentAutoRegister("tx-category-card-small", CategoryCardSmallComponent);

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "tx-category-card-small";
		m_Metadata.templatePath = "TraderX/datasets/gui/TraderXPrefab/CategoryCard/CategoryCardSmallView.layout";
	}
}
