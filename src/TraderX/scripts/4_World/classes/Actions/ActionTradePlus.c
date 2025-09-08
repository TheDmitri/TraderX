class ActionTraderXMenu: ActionInteractBase
{
	ref TraderXNpc npc;

	void ActionTraderXMenu()
	{
		m_CommandUID = DayZPlayerConstants.CMD_ACTIONMOD_INTERACTONCE;
		m_StanceMask = DayZPlayerConstants.STANCEMASK_ERECT | DayZPlayerConstants.STANCEMASK_CROUCH;
		m_HUDCursorIcon = CursorIcons.CloseHood;
	}

    override void CreateConditionComponents()
	{
		m_ConditionTarget = new CCTObject(8);
		m_ConditionItem = new CCINone;
	}

	override string GetText()
	{
		if(!npc)
			return "#tpm_trade";

		return string.Format("#tpm_trade - %1 (%2)", npc.givenName, npc.role);
	}

	override bool ActionCondition( PlayerBase player, ActionTarget target, ItemBase item )
    {
        if ( GetGame().IsServer() )
            return true;

		if(GetTraderXModule().IsTraderViewOpen())
			return false;

        PlayerBase playerNpc = PlayerBase.Cast( target.GetObject() );
		if(playerNpc && playerNpc.IsTraderNpc())
		{
			npc = GetTraderXModule().GetSettings().GetNpcById(playerNpc.GetNpcId());
			return npc != null;
		}

		BuildingBase nObject = BuildingBase.Cast( target.GetObject() );
		if (nObject && nObject.IsTraderNpc())
		{
			npc = GetTraderXModule().GetSettings().GetNpcById(nObject.GetNpcId());
			return npc != null;
		}

		return false;
    }

    override void OnStartClient(ActionData action_data)
    {
		TraderXTradingService.GetInstance().SetTraderNpc(npc);
		GetTraderXModule().OpenTraderView();
    }
}
