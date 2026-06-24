// TraderXBanking - Banking Page Shell
// Top-level component that hosts the tabs (Home/Account/Vault/Transfer/Ledger).
// Tab content is swapped through ComponentWidgetBinding driven by {{ current_tab }}.

class TXBBankingComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("txb-banking", TXBBankingComponent);

	// Active tab selector - drives ComponentWidgetBinding in BankingPageView.tabContent.
	// Valid values: banking-home-tab | banking-account-tab | banking-vault-tab |
	//               banking-transfer-tab | txb-ledger-tab
	string current_tab = "banking-home-tab";

	// Header bindings
	string bankerName;
	string subnavContext;
	string feesText;

	// Pending tab change (applied next frame — see SelectComponent).
	protected string m_PendingTab;

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "txb-banking";
		m_Metadata.templatePath = "TraderXBanking/datasets/layouts/BankingPageView.layout";
	}

	override void OnInit()
	{
		bankerName = "Banker";
		subnavContext = "SVETLOYARSK - CENTRAL BANK BRANCH";

		TXBSettings settings = GetTXBConfigService().GetClientSettings();
		float feePct = 0;
		if (settings) feePct = settings.transactionFeePercent;
		feesText = "FEE " + Math.Round(feePct * 100).ToString() + "%";

		TXBHomeTabComponent.Event_OnNavigateRequest.Insert(SelectComponent);
	}

	override void OnDestroy()
	{
		TXBHomeTabComponent.Event_OnNavigateRequest.Remove(SelectComponent);
	}

	// Called by NavButtonBinding when a tab / home nav card is clicked.
	// CRITICAL: the home nav cards live INSIDE the swappable tabContent. Swapping synchronously
	// here would free the home component while we're still inside its own button-click handler
	// (use-after-free -> native CTD). Defer the swap one frame so the click returns first; the
	// shell (this component) survives the swap, so it can safely tear the old content down.
	void SelectComponent(string selector)
	{
		if (current_tab == selector || m_PendingTab == selector)
			return;

		m_PendingTab = selector;
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(ApplyPendingTab, 1, false);
	}

	void ApplyPendingTab()
	{
		if (m_PendingTab == "" || current_tab == m_PendingTab)
			return;

		current_tab = m_PendingTab;
		m_PendingTab = "";
		Event_OnComponentChanged.Invoke(current_tab);
		MarkForCheck();
	}

	// Close button handler (X in the header).
	void OnClose()
	{
		TXBUIService svc = GetTXBUIService();
		if (svc)
			svc.CloseBankingMenu();
	}

	// Open the shared DayZ Beans Launcher promo (defined in TraderX core).
	void OnLauncher()
	{
#ifdef TraderX
		TXBeansPromoComponent.Open();
#endif
	}
}
