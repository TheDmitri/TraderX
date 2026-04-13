// TraderXGarage - ActionGarageMenu
// Interaction action for opening the garage menu when interacting with a garage NPC.

class ActionGarageMenuCB extends ActionContinuousBaseCB
{
	override void CreateActionComponent()
	{
		m_ActionData.m_ActionComponent = new CAContinuousTime(0.5);
	}
}

class ActionGarageMenu extends ActionInteractBase
{
	void ActionGarageMenu()
	{
		m_CommandUID = DayZPlayerConstants.CMD_ACTIONMOD_INTERACTONCE;
		m_StanceMask = DayZPlayerConstants.STANCEMASK_ERECT | DayZPlayerConstants.STANCEMASK_CROUCH;
		m_Text = "Open Garage";
	}

	override void CreateConditionComponents()
	{
		m_ConditionItem = new CCINone();
		m_ConditionTarget = new CCTCursor();
	}

	override typename GetInputType()
	{
		return ContinuousInteractActionInput;
	}

	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		if (!target)
			return false;

		Object targetObj = target.GetObject();
		if (!targetObj)
			return false;

		// Check if the target is a garage NPC (has TraderID matching TXG_GARAGE_NPC_ID)
		PlayerBase targetPlayer = PlayerBase.Cast(targetObj);
		if (targetPlayer && targetPlayer.m_TXGarageNpcId == TXG_GARAGE_NPC_ID)
			return true;

		BuildingBase targetBuilding = BuildingBase.Cast(targetObj);
		if (targetBuilding && targetBuilding.m_TXGarageNpcId == TXG_GARAGE_NPC_ID)
			return true;

		return false;
	}

	override void OnStartServer(ActionData action_data)
	{
		super.OnStartServer(action_data);

		PlayerBase player = action_data.m_Player;
		if (!player || !player.GetIdentity())
			return;

		Object targetObj = action_data.m_Target.GetObject();
		if (!targetObj)
			return;

		int playerLowId = GarageHelpers.GetLowSteamId(player.GetIdentity().GetPlainId());
		vector npcPos = targetObj.GetPosition();
		bool isFlag = false;

		// Send garage request RPC to server
		Param3<int, vector, bool> params = new Param3<int, vector, bool>(playerLowId, npcPos, isFlag);
		GetDayZularRPCManager().SendRPC(TXG_RPC_NAMESPACE, TXG_RPC_GARAGE_REQUEST, params, true, player.GetIdentity());
	}

	override void OnStartClient(ActionData action_data)
	{
		super.OnStartClient(action_data);

		PlayerBase player = action_data.m_Player;
		if (!player || !player.GetIdentity())
			return;

		Object targetObj = action_data.m_Target.GetObject();
		if (!targetObj)
			return;

		int playerLowId = GarageHelpers.GetLowSteamId(player.GetIdentity().GetPlainId());
		vector npcPos = targetObj.GetPosition();
		bool isFlag = false;

		// Send request and open UI
		Param3<int, vector, bool> params = new Param3<int, vector, bool>(playerLowId, npcPos, isFlag);
		GetDayZularRPCManager().SendRPC(TXG_RPC_NAMESPACE, TXG_RPC_GARAGE_REQUEST, params, true, null);

		GarageUIService uiService = GetGarageUIService();
		if (uiService)
			uiService.OpenGarageView(playerLowId, npcPos, isFlag);
	}
}
