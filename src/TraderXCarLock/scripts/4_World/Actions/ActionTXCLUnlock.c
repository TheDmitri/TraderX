// Unlock the vehicle. Single-use. Only for an authorized player on a locked car.

class ActionTXCLUnlock : ActionSingleUseBase
{
    bool m_IsBoat;

    void ActionTXCLUnlock()
    {
        m_CommandUID = DayZPlayerConstants.CMD_ACTIONMOD_INTERACTONCE;
        m_StanceMask = DayZPlayerConstants.STANCEMASK_CROUCH | DayZPlayerConstants.STANCEMASK_ERECT;
    }

    override void CreateConditionComponents()
    {
        m_ConditionTarget = new CCTNonRuined(7);
        m_ConditionItem = new CCINone;
    }

    override string GetText()
    {
        if (m_IsBoat)
            return "Unlock Boat";
        return "Unlock Car";
    }

    override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
    {
        if (GetGame().IsServer())
            return true;

        Object targetObject = target.GetObject();
        if (!targetObject)
            return false;

        m_IsBoat = false;
        if (BoatScript.Cast(targetObject))
            m_IsBoat = true;

        TXCLVehicleLockManager manager = TXCLVehicleLockManager.Get(targetObject);
        if (manager && player.CanInteractWithTXCLLock(manager) && manager.IsVehicleLocked())
            return true;

        return false;
    }

    override void OnStartServer(ActionData action_data)
    {
        TXCLVehicleLockManager manager = TXCLVehicleLockManager.Get(action_data.m_Target.GetObject());
        if (manager)
            manager.SetVehicleLock(false);
    }
}
