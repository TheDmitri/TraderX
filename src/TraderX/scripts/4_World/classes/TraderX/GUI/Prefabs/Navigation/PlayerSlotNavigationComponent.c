class PlayerSlotNavigationComponent extends DZlrComponentBase
{
    private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("traderx-player-nav", PlayerSlotNavigationComponent);
    private static PlayerSlotNavigationComponent active_instance;

    ref array<ref PlayerSlotNavigationButtonComponent> m_NavButtons = new array<ref PlayerSlotNavigationButtonComponent>();
    ref PlayerSlotNavigationButtonComponent selectedNavBtn;

    Widget player_nav_list;

    static PlayerSlotNavigationComponent GetNavigationInstance()
    {
        return active_instance;
    }

    PlayerSlotNavigationButtonComponent GetSelectedNavBtn()
    {
        return selectedNavBtn;
    }

    override protected void RegisterMetadata()
    {
        m_Metadata = new ComponentMetadata();
        m_Metadata.selector = "traderx-player-nav";
        m_Metadata.templatePath = "TraderX/datasets/gui/TraderXPrefab/Navigation/PlayerSlotNavigation.layout";
    }

    override void OnInit()
    {
        active_instance = this;
        PlayerSlotNavigationButtonComponent.Event_OnPlayerSlotEventClickCallBack.Insert(OnPlayerSlotEventClick);

        foreach (int navBtnId : EPlayerSlotNavigationButton.playerSlotNavigationButtons)
        {
            PlayerSlotNavigationButtonComponent btn = new PlayerSlotNavigationButtonComponent(navBtnId);
            btn.InitComponent(null);
            if (navBtnId == EPlayerSlotNavigationButton.ALL)
            {
                selectedNavBtn = btn;
            }
            else
            {
                btn.SetColor(0xff8b8f9a);
            }
            if (player_nav_list)
            {
                player_nav_list.AddChild(btn.GetRoot());
                btn.GetRoot().Show(true);
            }
            m_NavButtons.Insert(btn);
        }
    }

    override void OnDestroy()
    {
        PlayerSlotNavigationButtonComponent.Event_OnPlayerSlotEventClickCallBack.Remove(OnPlayerSlotEventClick);
        foreach (PlayerSlotNavigationButtonComponent btn : m_NavButtons)
        {
            if (btn && btn.GetRoot()) btn.GetRoot().Unlink();
        }
        m_NavButtons.Clear();
        active_instance = null;
    }

    void OnPlayerSlotEventClick(PlayerSlotNavigationButtonComponent playerSlotBtn, int command = ETraderXClickEvents.LCLICK)
    {
        if (command != ETraderXClickEvents.LCLICK)
            return;

        if (selectedNavBtn)
            selectedNavBtn.SetColor(0xff8b8f9a);

        selectedNavBtn = playerSlotBtn;
        if (selectedNavBtn)
            selectedNavBtn.SetColor(0xffaa4444);
    }

    override bool OnMouseEnter(Widget w, int x, int y)
    {
        if (w && w.GetUserID() >= EPlayerSlotNavigationButton.ALL && w.GetUserID() <= EPlayerSlotNavigationButton.VEHICLES)
            w.SetAlpha(1.0);
        return false;
    }

    override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
    {
        if (w && w.GetUserID() >= EPlayerSlotNavigationButton.ALL && w.GetUserID() <= EPlayerSlotNavigationButton.VEHICLES)
            w.SetAlpha(0.7);
        return false;
    }
}
