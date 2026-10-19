// TraderXGarage - UI Service
// Owns the garage menu lifecycle (open/close).

class TXGUIService extends DayZularService
{
    protected ref TXGGarageComponent m_Component;
    protected bool m_IsMenuOpen = false;

    void TXGUIService() {}

    bool IsMenuOpen()
    {
        return m_IsMenuOpen;
    }

    // ESC: leave the camera view first, close the window otherwise.
    void OnEscape()
    {
        if (m_Component && m_Component.IsSpotView())
        {
            m_Component.OnExitSpotView();
            return;
        }
        CloseGarageMenu();
    }

    // SelectPlayer (leaving the camera view) resets the player's input state: lock it again.
    void ReapplyInputLock()
    {
        if (!m_IsMenuOpen)
            return;
        GetGame().GetMission().PlayerControlDisable(INPUT_EXCLUDE_ALL);
        GetGame().GetUIManager().ShowUICursor(true);
        GetUApi().GetInputByID(UAUIMenu).ForceDisable(true);
    }

    bool HandleKey(int key)
    {
        if (!m_IsMenuOpen || !m_Component)
            return false;
        return m_Component.HandleSpotKey(key);
    }

    void OpenGarageMenu(int npcId = -1)
    {
        if (m_IsMenuOpen)
            return;

        // Ask the server for the current vehicle list (server reads our position itself).
        GetDayZularRPCManager().SendRPC(TXGConstants.RPC_NAMESPACE, "RequestGarage", null, true, null);

        m_Component = new TXGGarageComponent();
        m_Component.m_NpcId = npcId;    // which attendant we clicked -> park-out spot resolution
        m_Component.InitComponent(null);

        Widget root = m_Component.GetRoot();
        if (root)
        {
            root.Show(true);
            m_IsMenuOpen = true;

            GetGame().GetMission().PlayerControlDisable(INPUT_EXCLUDE_ALL);
            // ESC belongs to the garage while it is open (vanilla would also open the pause menu).
            GetUApi().GetInputByID(UAUIMenu).ForceDisable(true);
            GetGame().GetUIManager().ShowUICursor(true);
            GetGame().GetMission().GetHud().Show(false);
        }
    }

    void CloseGarageMenu()
    {
        if (!m_IsMenuOpen)
            return;

        if (m_Component)
        {
            m_Component.Destroy();
            m_Component = null;
        }

        m_IsMenuOpen = false;
        GetUApi().GetInputByID(UAUIMenu).ForceDisable(false);

        GetGame().GetMission().PlayerControlEnable(true);
        GetGame().GetUIManager().ShowUICursor(false);
        GetGame().GetMission().GetHud().Show(true);
    }
}

// Global accessor
static ref TXGUIService g_TXGUIService;

TXGUIService GetTXGUIService()
{
    if (!g_TXGUIService)
        g_TXGUIService = TXGUIService.Cast(DZlrServiceRegistry.GetInstance().GetService(TXGUIService));
    return g_TXGUIService;
}
