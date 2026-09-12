class TraderXAppComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("tx-app", TraderXAppComponent);

	private static ref TraderXAppComponent m_Instance;

	string trader_name;
	string trader_role;
	string wallet_amount;
	string currentPageSelector;
	// The page we swapped away from when opening a sub-page (e.g. customize), so
	// CloseSubPage can return to it. Empty = not in a sub-page.
	string m_PreviousPageSelector = "";
	// Page swap queued for next frame — see AddSubPage/CloseSubPage.
	private string m_PendingPageSelector = "";
	private bool m_PendingIsSubPage;

	ButtonWidget favBtn;
	ButtonWidget PurchaseBtn;
	ButtonWidget SellBtn;
	ButtonWidget CatalogBtn;
	ButtonWidget selectedBtn;

	// Loading overlay shown until the trader's products/stock arrive, so the menu opens
	// instantly instead of building a half-populated (frozen-looking) view.
	Widget loadingOverlay;
	string loading_text;
	private bool m_ContentShown;
	private int m_LoadingDots;
	// Safety valve: reveal the menu anyway if readiness never arrives (server hiccup).
	private const int LOADING_TIMEOUT_MS = 15000;

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "tx-app";
		m_Metadata.templatePath = "TraderX/datasets/gui/TraderXMainView.layout";
	}

	static TraderXAppComponent GetInstance()
	{
		return m_Instance;
	}

	override void OnInit()
	{
		m_Instance = this;
		AutoLoadWidgets();
		TraderXTradingService.Event_OnTraderXResponseReceived.Insert(OnTraderXResponseReceived);
		Setup();
	}

	override void OnDestroy()
	{
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(ApplyPendingPage);
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(TickLoadingText);
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(ShowContent);
		TraderXTradingService.Event_OnTraderXResponseReceived.Remove(OnTraderXResponseReceived);
		TraderXSelectionService.GetInstance().DeSelectAllItems();
		m_Instance = null;
	}

	// Footer "Sponsored By DayZ Beans Launcher" -> open the shared promo modal.
	void OnLauncherClick()
	{
		TXBeansPromoComponent.Open();
	}

	void Setup()
	{
		UpdateTraderInfo();
		UpdateWallet();

		// If the catalog is already cached (prewarmed), show content immediately; otherwise
		// hold on the loading overlay until the product/stock stream completes.
		if (TraderXTradingService.GetInstance().IsProductsReady())
		{
			ShowContent();
		}
		else
		{
			ShowLoading();
		}
	}

	void ShowLoading()
	{
		if (loadingOverlay) loadingOverlay.Show(true);
		m_LoadingDots = 0;
		TickLoadingText();
		// Reveal anyway after a timeout so a lost/late stream never leaves the player stuck.
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(ShowContent, LOADING_TIMEOUT_MS, false);
	}

	// Animate "Loading trader" → "." → ".." → "..." while waiting.
	void TickLoadingText()
	{
		if (m_ContentShown) return;
		m_LoadingDots = (m_LoadingDots + 1) % 4;
		string dots = "";
		int i;
		for (i = 0; i < m_LoadingDots; i++)
			dots = dots + ".";
		loading_text = Widget.TranslateString("#tpm_loading_trader") + dots;
		MarkForCheck();
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(TickLoadingText, 400, false);
	}

	void ShowContent()
	{
		if (m_ContentShown) return;
		m_ContentShown = true;
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(TickLoadingText);
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(ShowContent);
		if (loadingOverlay) loadingOverlay.Show(false);
		NavigateTo("tx-favorite-page", favBtn);
	}

	// Player's spendable money in the active trader's accepted currencies. Recomputed
	// on open and after every transaction response (buy/sell changes the balance).
	void UpdateWallet()
	{
		PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
		if (!player) return;
		TStringArray accepted = null;
		int npcId = TraderXTradingService.GetInstance().GetNpcId();
		TraderXNpc npc = GetTraderXModule().GetSettings().GetNpcById(npcId);
		if (npc) accepted = npc.GetCurrenciesAccepted();
		int money = TraderXCurrencyService.GetInstance().GetPlayerMoneyFromAllCurrency(player, accepted);
		wallet_amount = money.ToString();
		MarkForCheck();
	}

	void OnTraderXResponseReceived(int response, TraderXTransactionResultCollection transactionResultCollection = null)
	{
		UpdateWallet();
		// Stock arrives after the last product chunk -> catalog is ready, reveal the menu.
		if (!m_ContentShown && TraderXTradingService.GetInstance().IsProductsReady())
			ShowContent();
	}

	void UpdateTraderInfo()
	{
		int npcId = TraderXTradingService.GetInstance().GetNpcId();
		if (npcId == -1) return;
		TraderXNpc npc = GetTraderXModule().GetSettings().GetNpcById(npcId);
		if (!npc) return;
		trader_name = npc.GetGivenName();
		trader_role = npc.GetRole();
		MarkForCheck();
	}

	void NavigateTo(string pageSelector, ButtonWidget newSelectedBtn)
	{
		// A top-bar click wins over any sub-page swap still queued for next frame,
		// and leaves no sub-page to return to.
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(ApplyPendingPage);
		m_PendingPageSelector = "";
		m_PreviousPageSelector = "";

		ItemCardComponent.DestroyAllTooltips();
		TraderXSelectionService.GetInstance().DeSelectAllItems();

		if (selectedBtn)
		{
			selectedBtn.SetTextColor(TraderXTheme.NAV_INACTIVE);
			selectedBtn.SetAlpha(0.7);
		}
		selectedBtn = newSelectedBtn;
		if (selectedBtn)
		{
			selectedBtn.SetTextColor(TraderXTheme.NAV_ACTIVE);
			selectedBtn.SetAlpha(1.0);
		}

		currentPageSelector = pageSelector;
		MarkForCheck();
	}

	// Swap to the sub-page like any other page so the previous page (and its
	// components, e.g. tx-checkout) unmount cleanly. Remember it for CloseSubPage.
	//
	// CRITICAL: both AddSubPage and CloseSubPage are called from INSIDE the content
	// that the swap destroys — AddSubPage from an item card's right-click handler,
	// CloseSubPage from the customize page's own close button. Swapping synchronously
	// frees the component/widget whose click handler is still on the stack, and the
	// engine then returns into freed memory: native CTD, no VM exception, no crash log.
	// Defer one frame so the click fully unwinds first. Top-bar NavigateTo is safe
	// because those buttons live in the tx-app shell layout, not in swapped content.
	void AddSubPage(string pageSelector)
	{
		m_PendingPageSelector = pageSelector;
		m_PendingIsSubPage = true;
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(ApplyPendingPage, 1, false);
	}

	void CloseSubPage()
	{
		if (m_PreviousPageSelector == "")
			return;
		m_PendingPageSelector = m_PreviousPageSelector;
		m_PendingIsSubPage = false;
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(ApplyPendingPage, 1, false);
	}

	void ApplyPendingPage()
	{
		if (m_PendingPageSelector == "")
			return;

		if (m_PendingIsSubPage)
			m_PreviousPageSelector = currentPageSelector;
		else
			m_PreviousPageSelector = "";

		currentPageSelector = m_PendingPageSelector;
		m_PendingPageSelector = "";
		MarkForCheck();
	}

	void OnFavoriteClick()
	{
		TraderXInventoryManager.PlayMenuSound(ETraderXSounds.QUICK_EVENT);
		NavigateTo("tx-favorite-page", favBtn);
	}

	void OnPurchaseClick()
	{
		TraderXInventoryManager.PlayMenuSound(ETraderXSounds.QUICK_EVENT);
		NavigateTo("tx-purchase-page", PurchaseBtn);
	}

	void OnSellClick()
	{
		TraderXInventoryManager.PlayMenuSound(ETraderXSounds.QUICK_EVENT);
		NavigateTo("tx-sell-page", SellBtn);
	}

	void OnCatalogClick()
	{
		TraderXInventoryManager.PlayMenuSound(ETraderXSounds.QUICK_EVENT);
		NavigateTo("tx-catalog-page", CatalogBtn);
	}
}
