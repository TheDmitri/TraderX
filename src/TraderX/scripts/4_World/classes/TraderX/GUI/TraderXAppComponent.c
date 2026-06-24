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

	ButtonWidget favBtn;
	ButtonWidget PurchaseBtn;
	ButtonWidget SellBtn;
	ButtonWidget CatalogBtn;
	ButtonWidget selectedBtn;

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
		NavigateTo("tx-favorite-page", favBtn);
		UpdateTraderInfo();
		UpdateWallet();
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

	void AddSubPage(string pageSelector)
	{
		// Swap to the sub-page like any other page so the previous page (and its
		// components, e.g. tx-checkout) unmount cleanly. Remember it for CloseSubPage.
		m_PreviousPageSelector = currentPageSelector;
		currentPageSelector = pageSelector;
		MarkForCheck();
	}

	void CloseSubPage()
	{
		if (m_PreviousPageSelector != "")
			currentPageSelector = m_PreviousPageSelector;
		m_PreviousPageSelector = "";
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
