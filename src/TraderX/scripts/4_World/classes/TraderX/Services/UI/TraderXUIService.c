// TraderX UI Service - Manages the trader menu using DayZular ComponentMenu
class TraderXUIService : DayZularService
{
    private static ref TraderXUIService s_Instance;
    private ref ComponentMenu m_Menu;
    private ref TraderXMainComponent m_MainComponent;

    void TraderXUIService()
    {
        s_Instance = this;
    }

    static TraderXUIService GetInstance()
    {
        return s_Instance;
    }

    void OpenTraderView()
    {
        if (m_Menu && m_Menu.IsVisible())
        {
            return;
        }

        m_MainComponent = new TraderXMainComponent();
        m_MainComponent.InitComponent(null);

        m_Menu = new ComponentMenu();
        m_Menu.SetUseMouse(true);
        m_Menu.SetUseKeyboard(false);
        m_Menu.SetRootComponent(m_MainComponent);
        m_Menu.ShowMenu();
    }

    bool IsTraderViewOpen()
    {
        return m_Menu && m_Menu.IsVisible();
    }

    void CloseTraderView()
    {
        if (m_Menu)
        {
            TraderXTradingService.GetInstance().ClearNpc();
            m_Menu.HideMenu();
            delete m_Menu;
            m_Menu = null;
        }

        if (m_MainComponent)
        {
            delete m_MainComponent;
            m_MainComponent = null;
        }
    }

    TraderXMainComponent GetMainComponent()
    {
        return m_MainComponent;
    }
}
