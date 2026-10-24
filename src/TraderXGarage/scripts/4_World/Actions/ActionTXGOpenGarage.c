// TraderXGarage - Open Garage Action
// Interact with the garage attendant to open the garage menu.

class ActionTXGOpenGarage extends ActionInteractBase
{
    // Captured during ActionCondition (client) — the garage NPC the player is aiming at.
    protected int m_NpcId = -1;

    void ActionTXGOpenGarage()
    {
        m_CommandUID = DayZPlayerConstants.CMD_ACTIONMOD_INTERACTONCE;
        m_StanceMask = DayZPlayerConstants.STANCEMASK_ERECT | DayZPlayerConstants.STANCEMASK_CROUCH;
        m_HUDCursorIcon = CursorIcons.CloseHood;
        m_Text = "#txg_open_garage";
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
            return false;

        bool isGarage = false;
        m_NpcId = -1;

#ifdef TraderX
        PlayerBase targetPlayer = PlayerBase.Cast(targetObject);
        if (targetPlayer && GetTXGConfigService().IsGarageNpcId(targetPlayer.GetNpcId()))
        {
            isGarage = true;
            m_NpcId = targetPlayer.GetNpcId();
        }

        BuildingBase targetBuilding = BuildingBase.Cast(targetObject);
        if (targetBuilding && GetTXGConfigService().IsGarageNpcId(targetBuilding.GetNpcId()))
        {
            isGarage = true;
            m_NpcId = targetBuilding.GetNpcId();
        }
#endif
        if (!isGarage && GetTXGConfigService().IsGarageNpcClass(targetObject.GetType()))
            isGarage = true;

        return isGarage;
    }

    override void OnStartClient(ActionData action_data)
    {
        super.OnStartClient(action_data);

        if (GetTXGUIService().IsMenuOpen())
            return;

        GetTXGUIService().OpenGarageMenu(m_NpcId);
    }
}
