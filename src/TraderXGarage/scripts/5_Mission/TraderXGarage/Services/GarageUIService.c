// TraderXGarage - GarageUIService
// Client-side service managing the garage UI lifecycle.
// Opens/closes the garage menu, receives RPC responses, and relays data to the component.

class GarageUIService extends DayZularService
{
	private ref ComponentMenu m_GarageMenu;
	private ref GarageAppComponent m_GarageApp;

	void GarageUIService()
	{
		if (!GetGame().IsClient() && GetGame().IsMultiplayer())
			return;
	}

	// ─── Menu Lifecycle ───

	void OpenGarageView(int playerLowId, vector npcPosition, bool isFlag)
	{
		if (m_GarageMenu)
			CloseGarageView();

		m_GarageMenu = new ComponentMenu();
		m_GarageMenu.SetUseMouse(true);

		m_GarageApp = new GarageAppComponent();
		m_GarageApp.SetGarageData(playerLowId, npcPosition, isFlag);
		m_GarageApp.InitComponent(null);
		m_GarageMenu.SetRootComponent(m_GarageApp);
		m_GarageMenu.ShowMenu();
	}

	void CloseGarageView()
	{
		if (m_GarageMenu)
		{
			m_GarageMenu.HideMenu();
			m_GarageMenu = null;
		}
		m_GarageApp = null;
	}

	bool IsGarageMenuOpen()
	{
		return m_GarageMenu != null;
	}

	// ─── RPC Response Handling ───

	void OnGarageResponse(TStringArray vehicleNames, TIntArray vehicleHealths, vector parkingPosition, int responseCode)
	{
		if (!m_GarageApp)
			return;

		m_GarageApp.SetResponseData(vehicleNames, parkingPosition);
	}
}
