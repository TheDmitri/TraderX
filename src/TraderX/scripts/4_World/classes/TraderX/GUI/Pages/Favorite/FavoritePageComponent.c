class FavoritePageComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("tx-favorite-page", FavoritePageComponent);

	string search_keyword;
	ref array<ref TraderXCategory> _inputCategories = new array<ref TraderXCategory>();
	private bool m_IsUpdatingFavorites = false;

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "tx-favorite-page";
		m_Metadata.templatePath = "TraderX/datasets/gui/Favorites/FavoritesView.layout";
	}

	override void OnInit()
	{
		ItemCardComponent.Event_OnItemCardClickEventCallBack.Insert(OnItemCardClick);
		TraderXFavoritesService.Event_OnFavoriteChangedCallBack.Insert(OnFavoriteChanged);
		TraderXSearchBarComponent.Event_OnSearchChangedCallBack.Insert(OnSearchKeyword);
		Setup();
	}

	override void OnDestroy()
	{
		ItemCardComponent.Event_OnItemCardClickEventCallBack.Remove(OnItemCardClick);
		TraderXFavoritesService.Event_OnFavoriteChangedCallBack.Remove(OnFavoriteChanged);
		TraderXSearchBarComponent.Event_OnSearchChangedCallBack.Remove(OnSearchKeyword);
	}

	void Setup()
	{
		TraderXUINavigationService.GetInstance().SetNavigationId(ENavigationIds.FAVORITE);
		TraderXTradingService.GetInstance().SetTradeMode(ETraderXTradeMode.BUY);
		// tx-checkout mounts before this runs, so its OnInit read the stale trade mode —
		// refresh the button/label text now that BUY is set.
		if (CheckoutComponent.GetInstance()) CheckoutComponent.GetInstance().UpdateTradeModeText();
		ShowFavoriteList();
	}

	void ShowFavoriteList()
	{
		_inputCategories.Clear();
		array<ref TraderXCategory> categories = new array<ref TraderXCategory>();
		TraderXTradingService.GetInstance().GetFavoritesCategories(categories);
		foreach (TraderXCategory cat : categories)
			_inputCategories.Insert(cat);
		MarkForCheck();
	}

	// Debounced keyword pushed up from the embedded tx-search-bar component.
	void OnSearchKeyword(string keyword)
	{
		search_keyword = keyword;
		foreach (DZlrComponentBase child : m_ChildComponents)
		{
			CategoryCardComponent card = CategoryCardComponent.Cast(child);
			if (card)
				card.OnSearch(search_keyword);
		}
	}

	void OnItemCardClick(ItemCardComponent itemSelected, int command)
	{
		switch (command)
		{
			case ETraderXClickEvents.RCLICK:
				OnItemCardRClick(itemSelected);
				break;
			// Double-click-to-buy intentionally removed — too easy to buy by accident.
		}
	}

	void OnItemCardRClick(ItemCardComponent itemSelected)
	{
		TraderXProduct item = itemSelected.GetItem();
		if (!item.HasVariants() && !item.HasAttachments()) return;

		CustomizeStateService.GetInstance().Initialize(item);
		TraderXAppComponent.GetInstance().AddSubPage("tx-customize-page");
	}

	void OnItemCardDoubleClick(ItemCardComponent itemSelected)
	{
		if (TraderXTradingService.GetInstance().IsTransactionPending()) return;
		int npcId = TraderXTradingService.GetInstance().GetNpcId();
		TraderXTransactionCollection transactionCollection = new TraderXTransactionCollection();
		transactionCollection.AddTransaction(TraderXTransaction.CreateBuyTransaction(itemSelected._inputProduct, 1, itemSelected.GetPrice(), npcId));
		TraderXTradingService.GetInstance().LockTransaction();
		GetRPCManager().SendRPC("TraderX", "GetTransactionsRequest", new Param2<TraderXTransactionCollection, int>(transactionCollection, npcId));
	}

	void OnFavoriteChanged(TraderXFavoritesService favoritesService)
	{
		ShowFavoriteList();
	}
}
