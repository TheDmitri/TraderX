class PurchasePageComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("tx-purchase-page", PurchasePageComponent);

	string search_keyword;
	ref array<ref TraderXCategory> _inputCategories = new array<ref TraderXCategory>();
	bool m_AllExpanded = false;
	TextWidget toggleExpandText;
	ScrollWidget scrollCategory;

	// Player preview (inline playerPreview widget + strip/reset buttons in the layout).
	PlayerPreviewWidget playerPreview;
	protected PlayerBase entPlayerPreview;
	protected vector m_CharacterOrientation;

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
		m_CharacterOrientation = vector.Zero;
		InitPlayerPreview();
		Setup();
	}

	override void OnDestroy()
	{
		ItemCardComponent.Event_OnItemCardClickEventCallBack.Remove(OnItemCardClick);
		TraderXSearchBarComponent.Event_OnSearchChangedCallBack.Remove(OnSearchKeyword);
		TraderXSelectionService.Event_OnItemSelectionChanged.Remove(OnItemSelectionChanged);
		CategoryCardComponent.Event_OnCategoryCollapsedCallBack.Remove(OnCategoryCollapsed);
		if (entPlayerPreview)
			GetGame().ObjectDelete(entPlayerPreview);
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
		_inputCategories.Clear();
		array<ref TraderXCategory> categories = TraderXTradingService.GetInstance().GetTraderCategories();
		foreach (TraderXCategory category : categories)
		{
			if (!category || !category.isVisible) continue;
			_inputCategories.Insert(category);
		}
		MarkForCheck();
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
		ResetCategoryScroll();
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
