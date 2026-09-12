class CatalogPageComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("tx-catalog-page", CatalogPageComponent);

	string search_keyword;
	ref array<ref TraderXCategory> _inputCategories = new array<ref TraderXCategory>();
	bool m_AllExpanded = false;
	TextWidget toggleExpandText;
	ScrollWidget scrollCategory;
	// Expand-all is hidden while the trader's products stream in; a sync indicator shows instead.
	Widget toggleExpandBtn;
	Widget syncIndicator;

	string selected_item_name;
	string selected_sell_price;
	string selected_buy_price;
	string selected_stock;
	EntityAI selected_preview;
	string selected_description;
	ref TraderXProduct selected_item;

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "tx-catalog-page";
		m_Metadata.templatePath = "TraderX/datasets/gui/CatalogPage/CatalogPageView.layout";
	}

	override void OnInit()
	{
		AutoLoadWidgets();
		CatalogItemCardComponent.Event_OnCatalogItemCardClickCallBack.Insert(OnCatalogItemCardClick);
		TraderXSearchBarComponent.Event_OnSearchChangedCallBack.Insert(OnSearchKeyword);
		CatalogCategoryCardComponent.Event_OnCategoryCollapsedCallBack.Insert(OnCategoryCollapsed);
		TraderXTradingService.Event_OnTraderXResponseReceived.Insert(OnTradeResponse);
		SetSyncing(!TraderXTradingService.GetInstance().IsProductsReady());
		Setup();
	}

	override void OnDestroy()
	{
		CatalogItemCardComponent.Event_OnCatalogItemCardClickCallBack.Remove(OnCatalogItemCardClick);
		TraderXSearchBarComponent.Event_OnSearchChangedCallBack.Remove(OnSearchKeyword);
		CatalogCategoryCardComponent.Event_OnCategoryCollapsedCallBack.Remove(OnCategoryCollapsed);
		TraderXTradingService.Event_OnTraderXResponseReceived.Remove(OnTradeResponse);
	}

	// Hide expand-all + show the sync indicator until the trader's products finish streaming
	// (ALL_STOCK_RECEIVED — stock is sent after the last product chunk).
	void OnTradeResponse(int response, TraderXTransactionResultCollection results = null)
	{
		if (response == ETraderXResponse.ALL_STOCK_RECEIVED)
			SetSyncing(false);
	}

	void SetSyncing(bool syncing)
	{
		if (syncIndicator) syncIndicator.Show(syncing);
		if (toggleExpandBtn) toggleExpandBtn.Show(!syncing);
	}

	void ResetCategoryScroll()
	{
		if (scrollCategory)
			scrollCategory.VScrollToPos01(0.0);
	}

	void OnCategoryCollapsed()
	{
		ResetCategoryScroll();
	}

	void Setup()
	{
		TraderXUINavigationService.GetInstance().SetNavigationId(ENavigationIds.CATALOG);
		TraderXTradingService.GetInstance().SetTradeMode(ETraderXTradeMode.SELL);
		ShowCategoryList();
	}

	void ShowCategoryList()
	{
		_inputCategories.Clear();
		array<ref TraderXCategory> categories = TraderXTradingService.GetInstance().GetTraderCategories();
		foreach (TraderXCategory category : categories)
		{
			if (!category || !category.isVisible) continue;
			if (!HasSellableProducts(category)) continue;
			_inputCategories.Insert(category);
		}
		MarkForCheck();
	}

	bool HasSellableProducts(TraderXCategory category)
	{
		array<ref TraderXProduct> products = category.GetProducts();
		foreach (TraderXProduct product : products)
		{
			if (product && product.CanBeSold()) return true;
		}
		return false;
	}

	// Debounced keyword pushed up from the embedded tx-search-bar component.
	void OnSearchKeyword(string keyword)
	{
		search_keyword = keyword;
		foreach (DZlrComponentBase child : m_ChildComponents)
		{
			CatalogCategoryCardComponent card = CatalogCategoryCardComponent.Cast(child);
			if (card)
				card.OnSearch(search_keyword);
		}
		ResetCategoryScroll();
	}

	void OnToggleExpandClick()
	{
		TraderXInventoryManager.PlayMenuSound(ETraderXSounds.SELECT);
		ResetCategoryScroll();
		m_AllExpanded = !m_AllExpanded;
		// TODO: iterate child CatalogCategoryCardComponents via QueryChildren
		if (toggleExpandText)
		{
			if (m_AllExpanded)
				toggleExpandText.SetText(Widget.TranslateString("#tpm_collapse_all"));
			else
				toggleExpandText.SetText(Widget.TranslateString("#tpm_expand_all"));
		}
	}

	void OnCatalogItemCardClick(CatalogItemCardComponent itemSelected, int command)
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
			buyPrice = TraderXPresetsService.GetInstance().CalculateTotalPricePreset(selected_item.defaultPreset);
		selected_buy_price = buyPrice.ToString();

		selected_stock = TraderXProductStockRepository.GetStockDisplay(selected_item.productId, selected_item.maxStock);
		selected_description = itemSelected.GetItemDescription();
		if (selected_description == string.Empty)
			selected_description = "#tpm_no_description_available";

		MarkForCheck();
	}

	void OnCatalogItemCardRClick(CatalogItemCardComponent itemSelected)
	{
		TraderXProduct item = itemSelected.GetItem();
		if (!item.HasVariants() && !item.HasAttachments()) return;

		CustomizeStateService.GetInstance().Initialize(item);
		TraderXAppComponent.GetInstance().AddSubPage("tx-customize-page");
	}
}
