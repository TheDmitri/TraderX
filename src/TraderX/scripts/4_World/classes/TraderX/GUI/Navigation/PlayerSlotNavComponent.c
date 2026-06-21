class PlayerSlotNavComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("tx-player-slot-nav", PlayerSlotNavComponent);

	private static PlayerSlotNavComponent active_instance;

	ref array<ref TraderXNavSlotData> _inputNavBtnIds = new array<ref TraderXNavSlotData>();
	ref PlayerSlotBtnComponent selectedNavBtn;

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "tx-player-slot-nav";
		m_Metadata.templatePath = "TraderX/datasets/gui/TraderXPrefab/Navigation/PlayerSlotNavigation.layout";
	}

	static PlayerSlotNavComponent GetNavigationInstance()
	{
		return active_instance;
	}

	override void OnInit()
	{
		active_instance = this;
		PlayerSlotBtnComponent.Event_OnPlayerSlotEventClickCallBack.Insert(OnPlayerSlotEventClick);

		foreach (int navBtnId : EPlayerSlotNavigationButton.playerSlotNavigationButtons)
			_inputNavBtnIds.Insert(new TraderXNavSlotData(navBtnId));

		MarkForCheck();

		// Setup buttons after ForEach creates them
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(SetupButtons, 100);
	}

	override void OnDestroy()
	{
		PlayerSlotBtnComponent.Event_OnPlayerSlotEventClickCallBack.Remove(OnPlayerSlotEventClick);
		active_instance = null;
	}

	void SetupButtons()
	{
		foreach (int navBtnId : EPlayerSlotNavigationButton.playerSlotNavigationButtons)
		{
			PlayerSlotBtnComponent btn = PlayerSlotBtnComponent.GetPlayerSlotsButtonById(navBtnId);
			if (!btn) continue;
			if (navBtnId == EPlayerSlotNavigationButton.ALL)
			{
				// ALL is the default selection — colour it active up-front, else it
				// renders greyed until the first click re-selects it.
				btn.Setup(navBtnId, TraderXTheme.SLOT_ACTIVE);
				selectedNavBtn = btn;
			}
			else
			{
				btn.Setup(navBtnId, TraderXTheme.SLOT_INACTIVE);
			}
		}
	}

	PlayerSlotBtnComponent GetSelectedNavBtn()
	{
		return selectedNavBtn;
	}

	void SetItemCount(int navBtnId, int itemCount)
	{
		PlayerSlotBtnComponent btn = PlayerSlotBtnComponent.GetPlayerSlotsButtonById(navBtnId);
		if (btn) btn.SetItemCount(itemCount);
	}

	void OnPlayerSlotEventClick(PlayerSlotBtnComponent btn, int command)
	{
		if (command != ETraderXClickEvents.LCLICK) return;
		if (selectedNavBtn && selectedNavBtn.imgBtn)
			selectedNavBtn.imgBtn.SetColor(TraderXTheme.SLOT_INACTIVE);
		selectedNavBtn = btn;
		if (selectedNavBtn.imgBtn)
			selectedNavBtn.imgBtn.SetColor(TraderXTheme.SLOT_ACTIVE);
	}
}
