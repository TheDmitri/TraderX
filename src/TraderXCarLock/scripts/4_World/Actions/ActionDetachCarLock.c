class ActionDetachCarLockCB : ActionContinuousBaseCB
{
    override void CreateActionComponent()
    {
        m_ActionData.m_ActionComponent = new CAContinuousTime(3);
    }
}

class ActionDetachCarLock: ActionContinuousBase
{
    void ActionDetachCarLock()
    {
        m_CallbackClass = ActionDetachCarLockCB;
        m_CommandUID = DayZPlayerConstants.CMD_ACTIONFB_INTERACT;
        m_FullBody = true;
        m_StanceMask = DayZPlayerConstants.STANCEMASK_ERECT;
        m_SpecialtyWeight = UASoftSkillsWeight.PRECISE_LOW;
    }

    override void CreateConditionComponents()
    {
        m_ConditionTarget = new CCTNonRuined(7);
        m_ConditionItem = new CCINone;
    }

    override string GetText()
    {
        return "Detach Lock";
    }

    override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
    {
        if (GetGame().IsServer())
            return true;

        VehicleLockManager manager = VehicleLockManager.Get(target.GetObject());
        if (manager && !manager.HasPlayerInside() && manager.HasVehicleLock() && player.CanInteractWithVehicleLock(manager))
            return true;

        return false;
    }

    override void OnFinishProgressServer(ActionData action_data)
    {
        VehicleLockManager manager = VehicleLockManager.Get(action_data.m_Target.GetObject());
        if (manager)
        {
            manager.ResetVehicleLockData();
            ItemBase carLock = ItemBase.Cast(GetGame().CreateObjectEx(ItemBase.GetTXCarLockClassName(), action_data.m_Player.GetPosition(), ECE_PLACE_ON_SURFACE));
        }
    }
}
