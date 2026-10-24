class PurchasePageComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("tx-purchase-page", PurchasePageComponent);

	string search_keyword;
	ref array<ref TraderXCategory> _inputCategories = new array<ref TraderXCategory>();
	// Sidebar (Nasdara): every visible category; the main area shows the selected one.
	ref array<ref TraderXCategory> _navCategories = new array<ref TraderXCategory>();
	protected TraderXCategory m_SelectedCategory;
	// Remembered across tab switches within the session (by name: categories are rebuilt).
	static string s_SelectedCategoryName;
	bool m_AllExpanded = false;
	TextWidget toggleExpandText;
	ScrollWidget scrollCategory;
	// Expand-all is hidden while the trader's products stream in; a sync indicator shows instead.
	Widget toggleExpandBtn;
	Widget syncIndicator;
	ImageWidget fittingBtnIcon;
	bool m_FittingMode = false;

	// Player preview (inline playerPreview widget + strip/reset buttons in the layout).
	PlayerPreviewWidget playerPreview;
	protected PlayerBase entPlayerPreview;
	protected vector m_CharacterOrientation;

	// Preview show/hide toggle (top-bar button). Static so the choice survives page swaps
	// within the trader session.
	Widget PlayerPreviewPanel;
	ImageWidget previewToggleIcon;
	// The design has no preview column: hidden until the person button asks for it.
	static bool m_PreviewHidden = true;

	// Hover tooltip shared by the three preview action buttons (only one shows at a time).
	protected ref NavBtnTooltipComponent previewBtnTooltipComp;

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "tx-purchase-page";
		m_Metadata.templatePath = "TraderX/datasets/gui/PurchasePage/PurchasePageView.layout";
	}

	override void OnInit()
	{
		AutoLoadWidgets();
		ItemCardComponent.Event_OnItemCardClickEventCallBack.Insert(OnItemCardClick);
		TraderXSearchBarComponent.Event_OnSearchChangedCallBack.Insert(OnSearchKeyword);
		TraderXSelectionService.Event_OnItemSelectionChanged.Insert(OnItemSelectionChanged);
		CategoryCardComponent.Event_OnCategoryCollapsedCallBack.Insert(OnCategoryCollapsed);
		CategoryNavItemComponent.Event_OnCategoryNavClick.Insert(OnNavCategoryClick);
		TraderXTradingService.Event_OnTraderXResponseReceived.Insert(OnTradeResponse);
		// Reflect current stream state on mount (a tab opened after completion shows expand-all).
		SetSyncing(!TraderXTradingService.GetInstance().IsProductsReady());
		// Fitting/outfit mode is opt-in per visit — start off.
		m_FittingMode = false;
		TraderXSelectionService.SetFittingMode(false);
		UpdateFittingVisual();
		m_CharacterOrientation = vector.Zero;
		if (!m_PreviewHidden)
			InitPlayerPreview();
		Setup();
		ApplyPreviewVisibility();
	}

	void OnPreviewToggleClick()
	{
		TraderXInventoryManager.PlayMenuSound(ETraderXSounds.SELECT);
		m_PreviewHidden = !m_PreviewHidden;
		// Build the preview lazily the first time it's shown.
		if (!m_PreviewHidden && !entPlayerPreview)
			InitPlayerPreview();
		ApplyPreviewVisibility();
	}

	// Hide/show the preview column; the tiles take the freed width back (pixels, see layout).
	void ApplyPreviewVisibility()
	{
		bool visible = !m_PreviewHidden;

		if (PlayerPreviewPanel)
			PlayerPreviewPanel.Show(visible);

		if (scrollCategory)
		{
			// x/width are fractions of the page (layouts are width-relative, see
			// scripts/fluid-layouts.py): 341/1094 px of 1920 without the try-on preview,
			// 695/740 px with it. y/height are kept as read back: script pixel values are
			// not in the layout's units on scaled displays.
			float sx, sy, sw, sh;
			scrollCategory.GetPos(sx, sy);
			scrollCategory.GetSize(sw, sh);
			scrollCategory.SetPos(DZTernaryFloat.If(visible, 0.36198, 0.1776), sy);
			scrollCategory.SetSize(DZTernaryFloat.If(visible, 0.38542, 0.56979), sh);
		}

		UpdatePreviewToggleVisual();
	}

	void UpdatePreviewToggleVisual()
	{
		if (previewToggleIcon)
			previewToggleIcon.SetColor(DZTernaryInt.If(m_PreviewHidden, TraderXTheme.NAV_INACTIVE, TraderXTheme.SLOT_ACTIVE));
	}

	override void OnDestroy()
	{
		ItemCardComponent.Event_OnItemCardClickEventCallBack.Remove(OnItemCardClick);
		TraderXSearchBarComponent.Event_OnSearchChangedCallBack.Remove(OnSearchKeyword);
		TraderXSelectionService.Event_OnItemSelectionChanged.Remove(OnItemSelectionChanged);
		CategoryCardComponent.Event_OnCategoryCollapsedCallBack.Remove(OnCategoryCollapsed);
		CategoryNavItemComponent.Event_OnCategoryNavClick.Remove(OnNavCategoryClick);
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(ExpandSections);
		TraderXTradingService.Event_OnTraderXResponseReceived.Remove(OnTradeResponse);
		// Don't leak fitting mode into other pages.
		TraderXSelectionService.SetFittingMode(false);
		DestroyPreviewBtnTooltip();
		if (entPlayerPreview)
			GetGame().ObjectDelete(entPlayerPreview);
	}

	// === Preview action button tooltips ===

	void OnFittingBtnMouseEnter()
	{
		ShowPreviewBtnTooltip("Fitting mode: equip one item per slot to preview an outfit");
	}

	void OnStripBtnMouseEnter()
	{
		ShowPreviewBtnTooltip("Strip preview: remove all previewed items");
	}

	void OnResetBtnMouseEnter()
	{
		ShowPreviewBtnTooltip("Reset preview to your current gear");
	}

	void OnPreviewBtnMouseLeave()
	{
		DestroyPreviewBtnTooltip();
	}

	protected void ShowPreviewBtnTooltip(string text)
	{
		DestroyPreviewBtnTooltip();
		previewBtnTooltipComp = new NavBtnTooltipComponent();
		previewBtnTooltipComp.InitComponent(null);
		previewBtnTooltipComp.Setup(text);
	}

	protected void DestroyPreviewBtnTooltip()
	{
		if (previewBtnTooltipComp)
		{
			previewBtnTooltipComp.Destroy();
			previewBtnTooltipComp = null;
		}
	}

	// === Player preview ===

	protected void InitPlayerPreview()
	{
		CreatePreviewCharacter();
		SyncWithPlayerClothing();
	}

	protected void CreatePreviewCharacter()
	{
		if (entPlayerPreview)
			GetGame().ObjectDelete(entPlayerPreview);

		vector pos = GetGame().GetCurrentCameraPosition() - (GetGame().GetCurrentCameraDirection() * 0.5);
		pos[1] = GetGame().GetPlayer().GetPosition()[1];

		entPlayerPreview = PlayerBase.Cast(GetGame().CreateObjectEx(GetGame().GetPlayer().GetType(), pos, ECE_LOCAL|ECE_NOLIFETIME));
		if (entPlayerPreview && playerPreview)
		{
			playerPreview.SetPlayer(entPlayerPreview);
			UpdatePreviewPosition();
		}
	}

	protected void UpdatePreviewPosition()
	{
		if (!playerPreview || !entPlayerPreview) return;
		playerPreview.SetModelOrientation(m_CharacterOrientation);
		playerPreview.SetModelPosition(vector.Zero);
		playerPreview.SetSize(0.8, 0.8);
	}

	protected void SyncWithPlayerClothing()
	{
		PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
		if (!player || !entPlayerPreview) return;

		foreach (string slot : StaticTraderXCoreLists.playerAttachments)
		{
			EntityAI sourceItem = player.FindAttachmentBySlotName(slot);
			if (sourceItem)
			{
				if (slot == "Hand")
				{
					entPlayerPreview.GetHumanInventory().CreateInHands(sourceItem.GetType());
					continue;
				}
				int slotId = InventorySlots.GetSlotIdFromString(slot);
				if (slotId != -1)
					entPlayerPreview.GetInventory().CreateAttachmentEx(sourceItem.GetType(), slotId);
			}
		}
	}

	void OnItemSelectionChanged(TraderXProduct item)
	{
		if (!entPlayerPreview) return;

		EntityAI currentInHands, existingItem;
		string slotName = TraderXInventoryManager.GetSlotForPlayerPreview(item.className);
		if (slotName == string.Empty) return;

		if (item.isSelected)
		{
			existingItem = entPlayerPreview.FindAttachmentBySlotName(slotName);
			if (existingItem)
				GetGame().ObjectDelete(existingItem);

			if (slotName == "Hand")
			{
				currentInHands = entPlayerPreview.GetHumanInventory().GetEntityInHands();
				if (currentInHands)
					GetGame().ObjectDelete(currentInHands);
				entPlayerPreview.GetHumanInventory().CreateInHands(item.className);
			}
			else
			{
				entPlayerPreview.GetInventory().CreateAttachmentEx(item.className, InventorySlots.GetSlotIdFromString(slotName));
			}
		}
		else
		{
			if (slotName == "Hand")
			{
				currentInHands = entPlayerPreview.GetHumanInventory().GetEntityInHands();
				if (currentInHands && currentInHands.GetType() == item.className)
					GetGame().ObjectDelete(currentInHands);
			}
			else
			{
				existingItem = entPlayerPreview.FindAttachmentBySlotName(slotName);
				if (existingItem && existingItem.GetType() == item.className)
					GetGame().ObjectDelete(existingItem);
			}
		}

		playerPreview.UpdateItemInHands(entPlayerPreview.GetHumanInventory().GetEntityInHands());
	}

	void OnStripClick()
	{
		if (!entPlayerPreview) return;
		TraderXInventoryManager.PlayMenuSound(ETraderXSounds.SELECT);

		foreach (string slot : StaticTraderXCoreLists.playerAttachments)
		{
			EntityAI item = entPlayerPreview.FindAttachmentBySlotName(slot);
			if (item)
				GetGame().ObjectDelete(item);
		}

		EntityAI itemInHands = entPlayerPreview.GetHumanInventory().GetEntityInHands();
		if (itemInHands)
			GetGame().ObjectDelete(itemInHands);

		playerPreview.UpdateItemInHands(null);
	}

	void OnResetClick()
	{
		TraderXInventoryManager.PlayMenuSound(ETraderXSounds.SELECT);
		CreatePreviewCharacter();
		SyncWithPlayerClothing();
	}

	void Setup()
	{
		TraderXUINavigationService.GetInstance().SetNavigationId(ENavigationIds.PURCHASE);
		TraderXTradingService.GetInstance().SetTradeMode(ETraderXTradeMode.BUY);
		// The tx-checkout child mounts before this runs, so its OnInit read the stale
		// trade mode — re-sync the button/label now that BUY is set.
		if (CheckoutComponent.GetInstance()) CheckoutComponent.GetInstance().UpdateTradeModeText();
		ShowCategoryList();
	}

	void ShowCategoryList()
	{
		_navCategories.Clear();
		m_SelectedCategory = null;
		array<ref TraderXCategory> categories = TraderXTradingService.GetInstance().GetTraderCategories();
		foreach (TraderXCategory category : categories)
		{
			if (!category || !category.isVisible) continue;
			_navCategories.Insert(category);
			if (category.categoryName == s_SelectedCategoryName)
				m_SelectedCategory = category;
		}
		if (!m_SelectedCategory && _navCategories.Count() > 0)
			m_SelectedCategory = _navCategories[0];
		ShowSelectedCategory();
	}

	// Main area = the selected category, expanded (no accordion).
	void ShowSelectedCategory()
	{
		_inputCategories.Clear();
		if (m_SelectedCategory)
		{
			_inputCategories.Insert(m_SelectedCategory);
			s_SelectedCategoryName = m_SelectedCategory.categoryName;
		}
		CategoryNavItemComponent.SetSelected(m_SelectedCategory);
		MarkForCheck();
		ExpandSections();
		// the ForEach may build the section on the next frame
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(ExpandSections, 1, false);
		ResetCategoryScroll();
	}

	void ExpandSections()
	{
		foreach (DZlrComponentBase child : m_ChildComponents)
		{
			CategoryCardComponent card = CategoryCardComponent.Cast(child);
			if (!card) continue;
			if (!card.m_AlwaysExpanded)
				card.ExpandAlways();
			if (search_keyword != "")
				card.OnSearch(search_keyword);
		}
	}

	void OnNavCategoryClick(TraderXCategory category)
	{
		if (!category) return;
		m_SelectedCategory = category;
		search_keyword = "";
		ShowSelectedCategory();
	}

	// A keyword searches every category: one section per category with matches.
	void ShowSearchResults()
	{
		_inputCategories.Clear();
		string needle = search_keyword;
		needle.ToLower();
		foreach (TraderXCategory category : _navCategories)
		{
			// keep the returned array alive while iterating (1.30 frees temporaries)
			array<ref TraderXProduct> searchProducts = category.GetProducts();
			foreach (TraderXProduct item : searchProducts)
			{
				if (!item || !item.CanBeBought()) continue;
				string a = item.className;
				a.ToLower();
				string b = item.GetDisplayName();
				b.ToLower();
				if (a.Contains(needle) || b.Contains(needle))
				{
					_inputCategories.Insert(category);
					break;
				}
			}
		}
		MarkForCheck();
		ExpandSections();
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(ExpandSections, 1, false);
		ResetCategoryScroll();
	}

	// Snap the category list back to the top — the previous scroll offset is meaningless
	// once the list contents/heights change (search filter, collapse).
	void ResetCategoryScroll()
	{
		if (scrollCategory)
			scrollCategory.VScrollToPos01(0.0);
	}

	// A category was collapsed (its chevron / header) — return to the top.
	void OnCategoryCollapsed()
	{
		ResetCategoryScroll();
	}

	// Fitting/outfit mode: when on, selecting a clothing item is slot-exclusive in the cart
	// too, so the cart matches the dressed player preview (one item per slot). Off = multi-buy.
	void OnFittingToggleClick()
	{
		TraderXInventoryManager.PlayMenuSound(ETraderXSounds.SELECT);
		m_FittingMode = !m_FittingMode;
		TraderXSelectionService.SetFittingMode(m_FittingMode);
		UpdateFittingVisual();
	}

	void UpdateFittingVisual()
	{
		if (fittingBtnIcon)
			fittingBtnIcon.SetColor(DZTernaryInt.If(m_FittingMode, TraderXTheme.SLOT_ACTIVE, TraderXTheme.NAV_INACTIVE));
	}

	// Debounced keyword pushed up from the embedded tx-search-bar component.
	void OnSearchKeyword(string keyword)
	{
		search_keyword = keyword;
		if (search_keyword == "")
			ShowSelectedCategory();
		else
			ShowSearchResults();
	}

	// Products stream in per-trader on open; hide expand-all + show the sync indicator until
	// the catalog is complete (stock is sent after the last product chunk, so ALL_STOCK_RECEIVED
	// marks completion).
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

	void OnToggleExpandClick()
	{
		TraderXInventoryManager.PlayMenuSound(ETraderXSounds.SELECT);
		ResetCategoryScroll();
		m_AllExpanded = !m_AllExpanded;
		// isExpanded is inverted in CategoryCardComponent (true == collapsed),
		// so expand-all maps to SetExpanded(false) and collapse-all to SetExpanded(true).
		foreach (DZlrComponentBase child : m_ChildComponents)
		{
			CategoryCardComponent card = CategoryCardComponent.Cast(child);
			if (card)
				card.SetExpanded(!m_AllExpanded);
		}
		if (toggleExpandText)
		{
			if (m_AllExpanded)
				toggleExpandText.SetText(Widget.TranslateString("#tpm_collapse_all"));
			else
				toggleExpandText.SetText(Widget.TranslateString("#tpm_expand_all"));
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

		// Publish the selected product, then mount the customize page. The page's
		// OnInit reads the item back from CustomizeStateService.
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
}
