static ref TXCarLockUIService g_TXCarLockUIService;

TXCarLockUIService GetTXCarLockUIService()
{
    if (!g_TXCarLockUIService)
        g_TXCarLockUIService = TXCarLockUIService.Cast(DZlrServiceRegistry.GetInstance().GetService(TXCarLockUIService));
    return g_TXCarLockUIService;
}

class TXCarLockUIService : DayZularService
{
    private ref TXCarLockMenuComponent m_MenuComponent;
    private ref ComponentMenu m_ComponentMenu;

    void TXCarLockUIService()
    {
        g_TXCarLockUIService = this;
    }

    void OpenCarLockMenu(VehicleLockManager manager)
    {
        if (!GetGame().IsClient())
            return;

        if (g_Game.GetUIManager().GetMenu() != null)
            return;

        m_ComponentMenu = new ComponentMenu();
        m_ComponentMenu.SetUseMouse(true);

        m_MenuComponent = new TXCarLockMenuComponent();
        m_MenuComponent.SetManager(manager);
        m_MenuComponent.InitComponent(null);

        m_ComponentMenu.SetRootComponent(m_MenuComponent);
        m_ComponentMenu.ShowMenu();

        GetTXCarLockLogger().LogDebug("TXCarLockUIService: Menu opened");
    }

    void CloseCarLockMenu()
    {
        if (m_ComponentMenu)
        {
            m_ComponentMenu.CloseMenu();
            m_ComponentMenu = null;
            m_MenuComponent = null;
        }
    }

    void OnEnterPasswordAttemptResponse(TXEnterPasswordResponse response)
    {
        if (m_MenuComponent)
        {
            m_MenuComponent.OnEnterPasswordAttemptResponse(response);
        }
    }

    bool IsMenuOpen()
    {
        return m_ComponentMenu != null;
    }
}
