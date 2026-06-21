// TraderXBanking - Config Service
// Manages server config loading and client-side config sync

class TXBConfigService extends DayZularService
{
	protected ref TXBSettings m_ServerSettings;
	protected ref TXBSettings m_ClientSettings;

	void TXBConfigService()
	{
		if (GetGame().IsServer() || !GetGame().IsMultiplayer())
		{
			m_ServerSettings = TXBSettings.Load();
		}

		if (GetGame().IsClient() || !GetGame().IsMultiplayer())
		{
			m_ClientSettings = new TXBSettings();
		}
	}

	TXBSettings GetServerSettings()
	{
		return m_ServerSettings;
	}

	TXBSettings GetClientSettings()
	{
		return m_ClientSettings;
	}

	void SetClientSettings(TXBSettings settings)
	{
		m_ClientSettings = settings;
	}

	// Convenience: get the appropriate settings for current context
	TXBSettings GetSettings()
	{
		if (GetGame().IsServer() && m_ServerSettings)
			return m_ServerSettings;
		return m_ClientSettings;
	}
}

// Global accessor
static ref TXBConfigService g_TXBConfigService;

TXBConfigService GetTXBConfigService()
{
	if (!g_TXBConfigService)
		g_TXBConfigService = TXBConfigService.Cast(DZlrServiceRegistry.GetInstance().GetService(TXBConfigService));
	return g_TXBConfigService;
}
