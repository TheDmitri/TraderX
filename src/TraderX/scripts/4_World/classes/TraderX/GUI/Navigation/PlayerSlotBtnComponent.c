class PlayerSlotBtnComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("tx-player-slot-btn", PlayerSlotBtnComponent);

	static ref map<int, ref PlayerSlotBtnComponent> mPlayerSlotsButtons = new map<int, ref PlayerSlotBtnComponent>();
	static ref ScriptInvoker Event_OnPlayerSlotEventClickCallBack = new ScriptInvoker();

	ref NavBtnTooltipComponent navBtnTooltipComp;

	// Set by ForEachBinding (data-item) before OnInit. navBtnId is derived from it.
	ref TraderXNavSlotData _inputSlot;

	int navBtnId;
	string itemCount;
	string name;
	bool isLocked = false;
	private bool m_SuppressLeftSelect;

	Widget btnNav;
	ImageWidget imgBtn;
	ImageWidget imgLock;

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "tx-player-slot-btn";
		m_Metadata.templatePath = "TraderX/datasets/gui/TraderXPrefab/Navigation/PlayerSlotNavigationCard.layout";
	}

	override void OnInit()
	{
		AutoLoadWidgets();
		// Lock icon is opt-in: only shown after a right-click toggles isLocked.
		isLocked = false;
		if (imgLock) imgLock.Show(false);
		if (_inputSlot)
			navBtnId = _inputSlot.navBtnId;
		if (navBtnId > 0)
			mPlayerSlotsButtons.Set(navBtnId, this);
	}

	override void OnDestroy()
	{
		mPlayerSlotsButtons.Remove(navBtnId);
	}

	void Setup(int tNavBtnId, int color)
	{
		navBtnId = tNavBtnId;
		mPlayerSlotsButtons.Set(navBtnId, this);
		if (imgBtn)
		{
			imgBtn.LoadImageFile(0, EPlayerSlotNavigationButton.GetImageForBtnSlot(navBtnId));
			imgBtn.SetImage(0);
		}
		itemCount = string.Empty;
		MarkForCheck();

		if (color != -1)
		{
			if (imgBtn) imgBtn.SetColor(color);
			if (btnNav) btnNav.SetAlpha(1.0);
		}
	}

	void Hide()
	{
		if (btnNav) btnNav.Show(false);
	}

	static PlayerSlotBtnComponent GetPlayerSlotsButtonById(int id)
	{
		return mPlayerSlotsButtons[id];
	}

	bool IsLocked()
	{
		return isLocked;
	}

	void SetItemCount(int count)
	{
		itemCount = count.ToString();
		MarkForCheck();
	}

	// DayZ ButtonWidget OnClick fires for LEFT only; RIGHT arrives via OnRightClick
	// (data-on-rclick), hence the two separate handlers.
	void OnSlotClick()
	{
		// In this build a right-click also raises OnClick. OnSlotRightClick runs
		// first and sets this flag so the right-click only locks, never selects.
		if (m_SuppressLeftSelect)
		{
			m_SuppressLeftSelect = false;
			return;
		}
		Event_OnPlayerSlotEventClickCallBack.Invoke(this, ETraderXClickEvents.LCLICK);
		TraderXInventoryManager.PlayMenuSound(ETraderXSounds.QUICK_EVENT);
	}

	void OnSlotRightClick()
	{
		m_SuppressLeftSelect = true;
		// Self-heal if no OnClick follows (engine builds where OnClick is left-only).
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(ClearSuppressLeftSelect, 100);
		isLocked = !isLocked;
		if (imgLock) imgLock.Show(isLocked);
		Event_OnPlayerSlotEventClickCallBack.Invoke(this, ETraderXClickEvents.RCLICK);
	}

	void ClearSuppressLeftSelect()
	{
		m_SuppressLeftSelect = false;
	}

	void OnSlotMouseEnter()
	{
		string slotName = EPlayerSlotNavigationButton.GetSlotName(navBtnId);
		navBtnTooltipComp = new NavBtnTooltipComponent();
		navBtnTooltipComp.InitComponent(null);
		navBtnTooltipComp.Setup(slotName);
	}

	void OnSlotMouseLeave()
	{
		if (navBtnTooltipComp) { navBtnTooltipComp.Destroy(); navBtnTooltipComp = null; }
	}
}
