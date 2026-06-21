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

	void TraderXUIService()
	{
		g_TraderXUIService = this;
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

		TraderXTradingService.GetInstance().ClearNpc();
		m_TraderMenu.HideMenu();
		m_AppComponent = null;
		m_TraderMenu = null;
	}
}
