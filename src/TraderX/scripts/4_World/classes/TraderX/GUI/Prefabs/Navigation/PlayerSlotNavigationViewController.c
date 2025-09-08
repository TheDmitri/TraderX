class PlayerSlotNavigationViewController: ViewController
{
    private static PlayerSlotNavigationViewController active_instance;

    ref ObservableCollection<ref PlayerSlotNavigationButtonView> player_nav_list = new ObservableCollection<ref PlayerSlotNavigationButtonView>(this);

    ref PlayerSlotNavigationButtonViewController selectedNavBtn;

    static PlayerSlotNavigationViewController GetNavigationInstance()
    {
        return active_instance;
    }
    
    void PlayerSlotNavigationViewController()
    {
        PlayerSlotNavigationButtonViewController.Event_OnPlayerSlotEventClickCallBack.Insert(OnPlayerSlotEventClick);
    }

    PlayerSlotNavigationButtonViewController GetSelectedNavBtn()
    {
        return selectedNavBtn;
    }

    override void OnWidgetScriptInit(Widget w)
	{
		super.OnWidgetScriptInit(w);
		active_instance = this;
		foreach(int navBtnId: EPlayerSlotNavigationButton.playerSlotNavigationButtons)
        {
            if(navBtnId == EPlayerSlotNavigationButton.ALL){
                player_nav_list.Insert(new PlayerSlotNavigationButtonView(navBtnId));
                selectedNavBtn = player_nav_list.Get(0).GetTemplateController();
                continue;
            }

            player_nav_list.Insert(new PlayerSlotNavigationButtonView(navBtnId, 0xff8b8f9a));
        }
	}

    void SetItemCount(int navBtnId, int itemCount)
    {
        for(int i = 0; i < player_nav_list.GetArray().Count() ; i ++)
        {
            PlayerSlotNavigationButtonView navBtn = player_nav_list.Get(i);
            if(navBtn.GetTemplateController().navBtnId == navBtnId){
                navBtn.GetTemplateController().SetItemCount(itemCount);
            }
        }
    }

    void OnPlayerSlotEventClick(PlayerSlotNavigationButtonViewController playerSlotNavigationButtons, int command = ETraderXClickEvents.LCLICK)
    {
        if(command != ETraderXClickEvents.LCLICK)
            return;
            
        if(selectedNavBtn){
            selectedNavBtn.imgBtn.SetColor(0xff8b8f9a);
        }
        selectedNavBtn = playerSlotNavigationButtons;
        selectedNavBtn.imgBtn.SetColor(0xffaa4444);
    }

    override bool OnMouseEnter(Widget w, int x, int y)
	{
        if(w && w.GetUserID() >= EPlayerSlotNavigationButton.ALL && w.GetUserID() <= EPlayerSlotNavigationButton.ARMBAND){
            w.SetAlpha(1.0);
        }
        return false;
	}

    override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
        if(w && w.GetUserID() >= EPlayerSlotNavigationButton.ALL && w.GetUserID() <= EPlayerSlotNavigationButton.ARMBAND){
            w.SetAlpha(0.7);
        }
        return false;
	}
}