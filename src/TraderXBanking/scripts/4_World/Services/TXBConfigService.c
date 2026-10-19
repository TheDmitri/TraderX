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
			// Migrate a TraderPlus V1 banking config first (no-op if none / already configured).
			oldTraderPlusBankingConfig.ConvertIfPresent();
			m_ServerSettings = TXBSettings.Load();
		}

		if (GetGame().IsClient() || !GetGame().IsMultiplayer())
		{
			m_ClientSettings = new TXBSettings();
		}
	}

	protected bool m_CurrenciesSyncedFromCore = false;

	TXBSettings GetServerSettings()
	{
#ifdef TraderX
		if (m_ServerSettings && !m_CurrenciesSyncedFromCore)
		{
			SyncCurrenciesFromTraderX();
		}
#endif
		return m_ServerSettings;
	}

#ifdef TraderX
	protected void SyncCurrenciesFromTraderX()
	{
		TraderXCurrencyService curSvc = TraderXCurrencyService.m_instance;
		if (!curSvc || !curSvc.currencySettings || !curSvc.currencySettings.currencyTypes)
			return;
		m_CurrenciesSyncedFromCore = true;
		foreach (TraderXCurrencyType curType : curSvc.currencySettings.currencyTypes)
		{
			if (!curType || !curType.currencies)
				continue;
			foreach (TraderXCurrency cur : curType.currencies)
			{
				if (!cur || cur.className == "" || cur.value <= 0)
					continue;
				if (!m_ServerSettings.IsCurrency(cur.className))
				{
					m_ServerSettings.currencies.Insert(new TXBCurrencyEntry(cur.className, cur.value));
				}
			}
		}
	}
#endif

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
