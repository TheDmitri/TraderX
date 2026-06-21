// TraderXBanking - Open Bank Action
// Context action to open the banking menu (interact with NPC/building with TXBConstants.BANK_ID)

class ActionTXBOpenBank extends ActionInteractBase
{
	void ActionTXBOpenBank()
	{
		m_CommandUID = DayZPlayerConstants.CMD_ACTIONMOD_INTERACTONCE;
		m_StanceMask = DayZPlayerConstants.STANCEMASK_ERECT | DayZPlayerConstants.STANCEMASK_CROUCH;
		m_HUDCursorIcon = CursorIcons.CloseHood;
		m_Text = "Open Bank";
	}

	override void CreateConditionComponents()
	{
		m_ConditionTarget = new CCTObject(8);
		m_ConditionItem = new CCINone;
	}

	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		if (GetGame().IsServer())
			return true;

		Object targetObject = target.GetObject();
		if (!targetObject)
		{
			return false;
		}

		// Check if target entity is a bank (identified by TXBConstants.BANK_ID)
		bool isBank = false;

		// PlayerBase NPC check - only when TraderX is present (provides GetNpcId)
#ifdef TraderX
		PlayerBase targetPlayer = PlayerBase.Cast(targetObject);
		if (targetPlayer && targetPlayer.GetNpcId() == TXBConstants.BANK_ID)
			isBank = true;
#endif

		// BuildingBase check - works in both modes (TXBBuildingBase provides GetNpcId in standalone)
		BuildingBase targetBuilding = BuildingBase.Cast(targetObject);
		if (targetBuilding && targetBuilding.GetNpcId() == TXBConstants.BANK_ID){
			isBank = true;
		}

		if (!isBank)
			return false;

		// Check credit card requirement
		TXBSettings clientSettings = GetTXBConfigService().GetClientSettings();
		if (clientSettings && clientSettings.requireCreditCard)
		{
			ItemBase inHands = player.GetItemInHands();
			if (!inHands || !inHands.IsKindOf("TXBCreditCard"))
			{
				return false;
			}
		}

		return true;
	}

	override void OnStartClient(ActionData action_data)
	{
		super.OnStartClient(action_data);

		if (GetTXBUIService().IsMenuOpen())
			return;

		GetTXBUIService().OpenBankingMenu();
	}
}
