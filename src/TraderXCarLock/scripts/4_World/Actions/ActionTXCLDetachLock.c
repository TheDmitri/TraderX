#ifndef CarLock
// TraderPlus V1 defines "CarLock" and owns the CarLock item: TraderX CarLock stands down when it is loaded (see TXCLTraderPlusNotice).
// Detach the lock from a vehicle (authorized players only). Continuous; on finish resets the
// lock data and drops a lock item back on the ground.

class ActionTXCLDetachLockCB : ActionContinuousBaseCB
{
    override void CreateActionComponent()
    {
        m_ActionData.m_ActionComponent = new CAContinuousTime(3);
    }
}

class ActionTXCLDetachLock : ActionContinuousBase
{
    void ActionTXCLDetachLock()
    {
        m_CallbackClass = ActionTXCLDetachLockCB;
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

        TXCLVehicleLockManager manager = TXCLVehicleLockManager.Get(target.GetObject());
        if (manager && !manager.HasPlayerInside() && manager.HasVehicleLock() && player.CanInteractWithTXCLLock(manager))
            return true;

        return false;
    }

    override void OnFinishProgressServer(ActionData action_data)
    {
        TXCLVehicleLockManager manager = TXCLVehicleLockManager.Get(action_data.m_Target.GetObject());
        if (manager)
        {
            manager.ResetVehicleLockData();
            GetGame().CreateObjectEx(ItemBase.GetTXCLLockClassName(), action_data.m_Player.GetPosition(), ECE_PLACE_ON_SURFACE);
        }
    }
}
#endif
