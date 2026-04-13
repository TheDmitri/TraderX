class PlayerSlotNavigationButtonComponent extends DZlrComponentBase
{
    private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("traderx-player-slot-btn", PlayerSlotNavigationButtonComponent);

    static ref map<int, ref PlayerSlotNavigationButtonComponent> mPlayerSlotsButtonsController = new map<int, ref PlayerSlotNavigationButtonComponent>();
    static ref ScriptInvoker Event_OnPlayerSlotEventClickCallBack = new ScriptInvoker();

    ref NavBtnTooltipComponent navBtnTooltipView;

    int navBtnId;
    Widget btnNav;
    ImageWidget imgBtn;
    ImageWidget imgLock;
    TextWidget itemCountText;
    string itemCount;
    private bool isLocked = false;

    void PlayerSlotNavigationButtonComponent(int tNavBtnId)
    {
        navBtnId = tNavBtnId;
    }

    override protected void RegisterMetadata()
    {
        m_Metadata = new ComponentMetadata();
        m_Metadata.selector = "traderx-player-slot-btn";
        m_Metadata.templatePath = "TraderX/datasets/gui/TraderXPrefab/Navigation/PlayerSlotNavigationCard.layout";
    }

    override void OnInit()
    {
        mPlayerSlotsButtonsController.Set(navBtnId, this);
        if (imgBtn) imgBtn.LoadImageFile(0, EPlayerSlotNavigationButton.GetImageForBtnSlot(navBtnId));
        itemCount = string.Empty;
        if (itemCountText) itemCountText.SetText(itemCount);
    }

    override void OnDestroy()
    {
        mPlayerSlotsButtonsController.Remove(navBtnId);
    }

    static PlayerSlotNavigationButtonComponent GetPlayerSlotsButtonsControllerById(int id)
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
        if (itemCountText) itemCountText.SetText(itemCount);
    }

    void SetColor(int color)
    {
        if (imgBtn) imgBtn.SetColor(color);
        if (btnNav) btnNav.SetAlpha(1.0);
    }

    void Hide()
    {
        if (btnNav) btnNav.Show(false);
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (w == btnNav && button == MouseState.LEFT)
        {
            Event_OnPlayerSlotEventClickCallBack.Invoke(this, ETraderXClickEvents.LCLICK);
            TraderXInventoryManager.PlayMenuSound(ETraderXSounds.QUICK_EVENT);
            return true;
        }

        if (w == btnNav && button == MouseState.RIGHT)
        {
            isLocked = !isLocked;
            if (imgLock) imgLock.Show(isLocked);
            Event_OnPlayerSlotEventClickCallBack.Invoke(this, ETraderXClickEvents.RCLICK);
            return true;
        }

        return false;
    }

    override bool OnMouseEnter(Widget w, int x, int y)
    {
        if (w && w == btnNav)
        {
            string name = EPlayerSlotNavigationButton.GetSlotName(navBtnId);
            navBtnTooltipView = new NavBtnTooltipComponent(name, 10.0, 10.0);
            navBtnTooltipView.InitComponent(null);
        }
        return false;
    }

    override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
    {
        if (w && w == btnNav)
        {
            delete navBtnTooltipView;
            navBtnTooltipView = null;
        }
        return false;
    }
}
