class PlayerSlotNavigationButtonViewController: ViewController
{
    static ref map<int, ref PlayerSlotNavigationButtonViewController> mPlayerSlotsButtonsController = new map<int, ref PlayerSlotNavigationButtonViewController>();
    
    static ref ScriptInvoker Event_OnPlayerSlotEventClickCallBack = new ScriptInvoker();

    ref NavBtnTooltipView  navBtnTooltipView;

    int navBtnId;

    Widget btnNav;
    ImageWidget imgBtn;
    ImageWidget imgLock;
    string itemCount;

    private bool isLocked = false;

    string name;

    void Setup(int tNavBtnId, int color)
    {
        this.navBtnId = tNavBtnId;
        mPlayerSlotsButtonsController.Set(this.navBtnId, this);
        imgBtn.LoadImageFile(0, EPlayerSlotNavigationButton.GetImageForBtnSlot(navBtnId));
        itemCount = string.Empty;
        NotifyPropertyChanged("itemCount");

        if(color != -1){
            imgBtn.SetColor(color);
            btnNav.SetAlpha(1.0);
        }
    }

    void Hide()
    {
        btnNav.Show(false);
    }

    void ~PlayerSlotNavigationButtonViewController()
    {
        mPlayerSlotsButtonsController.Remove(navBtnId);
    }

    static PlayerSlotNavigationButtonViewController GetPlayerSlotsButtonsControllerById(int id)
    {
        return mPlayerSlotsButtonsController[id];
    }

    bool IsLocked()
    {
        return isLocked;
    }

    void SetItemCount(int count)
    {
        itemCount = count.ToString();
        NotifyPropertyChanged("itemCount");
    }

    override bool OnClick(Widget w, int x, int y, int button)
	{
        if (w == btnNav && button == MouseState.LEFT){
            Event_OnPlayerSlotEventClickCallBack.Invoke(this,  ETraderXClickEvents.LCLICK);
            TraderXInventoryManager.PlayMenuSound(ETraderXSounds.QUICK_EVENT);
            return true;
        }

        if (w == btnNav && button == MouseState.RIGHT){
            isLocked = !isLocked;
            imgLock.Show(isLocked);
            Event_OnPlayerSlotEventClickCallBack.Invoke(this,  ETraderXClickEvents.RCLICK);
            return true;
        }

        return false;          
	}

    override bool OnMouseEnter(Widget w, int x, int y)
	{
        if(w && w == btnNav){
            string name = EPlayerSlotNavigationButton.GetSlotName(navBtnId);
            navBtnTooltipView = new NavBtnTooltipView(name, 10.0, 10.0);
        }
        return false;
	}

    override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
        if(w && w == btnNav){
            delete navBtnTooltipView;
        }
        return false;
	}
}