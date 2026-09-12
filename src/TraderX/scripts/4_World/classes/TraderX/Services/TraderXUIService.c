static ref TraderXUIService g_TraderXUIService;

TraderXUIService GetTraderXUIService()
{
	if (!g_TraderXUIService)
		g_TraderXUIService = TraderXUIService.Cast(DZlrServiceRegistry.GetInstance().GetService(TraderXUIService));
	return g_TraderXUIService;
}

class TraderXUIService : DayZularService
{
	private ref ComponentMenu m_TraderMenu;
	private ref TraderXAppComponent m_AppComponent;

	private ref ComponentMenu m_LicenseModalMenu;
	private ref TraderXLicenseModalComponent m_LicenseModalComp;

	void TraderXUIService()
	{
		g_TraderXUIService = this;
	}

	// License-locked trader warning: own menu, since the trader view never opens.
	void OpenLicenseModal(array<string> requiredLicenses)
	{
		if (m_LicenseModalMenu && m_LicenseModalMenu.IsVisible())
			return;

		m_LicenseModalMenu = new ComponentMenu();
		m_LicenseModalMenu.SetUseMouse(true);
		m_LicenseModalComp = new TraderXLicenseModalComponent();
		m_LicenseModalComp.InitComponent(null);
		m_LicenseModalComp.Setup(requiredLicenses);
		m_LicenseModalMenu.SetRootComponent(m_LicenseModalComp);
		m_LicenseModalMenu.ShowMenu();
	}

	void CloseLicenseModal()
	{
		if (!m_LicenseModalMenu)
			return;

		m_LicenseModalMenu.HideMenu();
		m_LicenseModalComp = null;
		m_LicenseModalMenu = null;
	}

	void OpenTraderView()
	{
		if (m_TraderMenu && m_TraderMenu.IsVisible())
			return;

		m_TraderMenu = new ComponentMenu();
		m_TraderMenu.SetUseMouse(true);
		m_AppComponent = new TraderXAppComponent();
		m_AppComponent.InitComponent(null);
		m_TraderMenu.SetRootComponent(m_AppComponent);
		m_TraderMenu.ShowMenu();
	}

	bool IsTraderViewOpen()
	{
		return m_TraderMenu && m_TraderMenu.IsVisible();
	}

	void CloseTraderView()
	{
		if (!m_TraderMenu)
			return;

		ItemCardComponent.DestroyAllTooltips();
		TraderXTradingService.GetInstance().ClearNpc();
		m_TraderMenu.HideMenu();
		m_AppComponent = null;
		m_TraderMenu = null;
	}
}
