// TraderXBanking - Open Bank Action
// Context action to open the banking menu (interact with NPC/building with TXB_BANK_ID)

class ActionTXBOpenBank extends ActionInteractBase
{
	void ActionTXBOpenBank()
	{
		m_CommandUID = DayZPlayerConstants.CMD_ACTIONMOD_INTERACTONCE;
		m_StanceMask = DayZPlayerConstants.STANCEMASK_ERECT | DayZPlayerConstants.STANCEMASK_CROUCH;
		m_Text = "Open Bank";
	}

	override typename GetInputType()
	{
		return ContinuousInteractActionInput;
	}

	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		if (GetGame().IsServer())
			return true;

		Object targetObject = target.GetObject();
		if (!targetObject)
			return false;

		// Check if target entity is a bank (identified by TXB_BANK_ID variable)
		bool isBank = false;

		PlayerBase targetPlayer = PlayerBase.Cast(targetObject);
		if (targetPlayer)
		{
			int playerBankId = -999;
			targetPlayer.GetVariableByName("TraderID", playerBankId);
			if (playerBankId == TXB_BANK_ID)
				isBank = true;
		}

		BuildingBase targetBuilding = BuildingBase.Cast(targetObject);
		if (targetBuilding)
		{
			int buildingBankId = -999;
			targetBuilding.GetVariableByName("TraderID", buildingBankId);
			if (buildingBankId == TXB_BANK_ID)
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
				return false;
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
