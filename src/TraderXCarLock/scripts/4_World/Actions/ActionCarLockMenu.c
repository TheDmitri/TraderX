class ActionCarLockMenu: ActionInteractBase
{
    void ActionCarLockMenu()
    {
        m_CommandUID = DayZPlayerConstants.CMD_ACTIONMOD_INTERACTONCE;
        m_StanceMask = DayZPlayerConstants.STANCEMASK_ERECT | DayZPlayerConstants.STANCEMASK_CROUCH;
        m_HUDCursorIcon = CursorIcons.CloseHood;
    }

    override void CreateConditionComponents()
    {
        m_ConditionTarget = new CCTObject(7);
        m_ConditionItem = new CCINone;
    }

    override string GetText()
    {
        return "Open Lock Menu";
    }

    override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
    {
        if (GetGame().IsServer())
            return true;

        VehicleLockManager manager = VehicleLockManager.Get(target.GetObject());
        if (manager && !player.CanInteractWithVehicleLock(manager) && manager.HasVehicleLock())
            return true;

        return false;
    }

    override void OnStartClient(ActionData action_data)
    {
        VehicleLockManager manager = VehicleLockManager.Get(action_data.m_Target.GetObject());
        if (manager)
        {
            GetTXCarLockUIService().OpenCarLockMenu(manager);
        }
    }
}
