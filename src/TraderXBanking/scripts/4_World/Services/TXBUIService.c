// TraderXBanking - UI Service
// Manages the banking menu lifecycle (open/close/toggle)

class TXBUIService extends DayZularService
{
	protected ref TXBBankingComponent m_BankingComponent;
	protected bool m_IsMenuOpen = false;

	void TXBUIService() {}

	bool IsMenuOpen()
	{
		return m_IsMenuOpen;
	}

	void OpenBankingMenu()
	{
		if (m_IsMenuOpen)
			return;

		// Request fresh account data from server
		GetDayZularRPCManager().SendRPC(TXBConstants.RPC_NAMESPACE, "RequestAccount", null, true, null);

		m_BankingComponent = new TXBBankingComponent();
		m_BankingComponent.InitComponent(null);

		Widget root = m_BankingComponent.GetRoot();
		if (root)
		{
			root.Show(true);
			m_IsMenuOpen = true;

			// Enable mouse cursor
			GetGame().GetMission().PlayerControlDisable(INPUT_EXCLUDE_ALL);
			GetGame().GetUIManager().ShowUICursor(true);
			GetGame().GetMission().GetHud().Show(false);
		}
	}

	void CloseBankingMenu()
	{
		if (!m_IsMenuOpen)
			return;

		if (m_BankingComponent)
		{
			m_BankingComponent.Destroy();
			m_BankingComponent = null;
		}

		m_IsMenuOpen = false;

		// Restore player controls
		GetGame().GetMission().PlayerControlEnable(true);
		GetGame().GetUIManager().ShowUICursor(false);
		GetGame().GetMission().GetHud().Show(true);
	}

	void PlayMenuSound(string soundSet)
    {
        if(!GetGame().IsClient())
        return;

        EffectSound sound;
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        player.PlaySoundSet(sound, soundSet, 0.0, 0.0);
    }
}

// Global accessor
static ref TXBUIService g_TXBUIService;

TXBUIService GetTXBUIService()
{
	if (!g_TXBUIService)
		g_TXBUIService = TXBUIService.Cast(DZlrServiceRegistry.GetInstance().GetService(TXBUIService));
	return g_TXBUIService;
}
