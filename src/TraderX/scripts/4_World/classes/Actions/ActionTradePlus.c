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
			return CanTrade(npc);
		}

		BuildingBase nObject = BuildingBase.Cast( target.GetObject() );
		if (nObject && nObject.IsTraderNpc())
		{
			npc = GetTraderXModule().GetSettings().GetNpcById(nObject.GetNpcId());
			return CanTrade(npc);
		}

		return false;
    }

	// Service NPCs (bank ATM, garage attendant) are spawned via the trader list but carry no
	// categories — they only expose their own menu action, never the trade menu.
	bool CanTrade(TraderXNpc n)
	{
		return n && n.categoriesId && n.categoriesId.Count() > 0;
	}

    override void OnStartClient(ActionData action_data)
    {
		// License-locked trader: show the warning modal instead of the trade view.
		// The server also re-checks licenses on every transaction.
		if (!TraderXLicenseService.GetInstance().HasLicenses(npc.GetLicensesRequired()))
		{
			if (GetTraderXUIService())
				GetTraderXUIService().OpenLicenseModal(npc.GetLicensesRequired());
			return;
		}

		TraderXTradingService.GetInstance().SetTraderNpc(npc);
		GetTraderXModule().OpenTraderView();
    }
}
