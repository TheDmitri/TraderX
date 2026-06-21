// TraderXBanking - Banking Page Shell
// Top-level component that hosts the three tabs (Account/Transfer/Ledger).
// Tab content is swapped through ComponentWidgetBinding driven by {{ current_tab }}.

class TXBBankingComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("txb-banking", TXBBankingComponent);

	// Active tab selector — drives ComponentWidgetBinding in BankingPageView.tabContent.
	// Valid values: banking-home-tab | banking-account-tab | banking-vault-tab |
	//               banking-transfer-tab | banking-clan-tab | txb-ledger-tab
	string current_tab = "banking-home-tab";

	// Header bindings
	string bankerName;
	string subnavContext;
	string feesText;

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "txb-banking";
		m_Metadata.templatePath = "TraderXBanking/datasets/layouts/BankingPageView.layout";
	}

	override void OnInit()
	{
		bankerName = "Banker";
		subnavContext = "SVETLOYARSK · CENTRAL BANK BRANCH";

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

	// Called by NavButtonBinding when a tab button is clicked.
	void SelectComponent(string selector)
	{
		if (current_tab == selector)
			return;

		current_tab = selector;
		Event_OnComponentChanged.Invoke(selector);
		MarkForCheck();
	}

	// Close button handler (X in the header).
	void OnClose()
	{
		TXBUIService svc = GetTXBUIService();
		if (svc)
			svc.CloseBankingMenu();
	}
}
