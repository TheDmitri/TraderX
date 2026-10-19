#ifndef CarLock
// TraderPlus V1 defines "CarLock" and owns the CarLock item: TraderX CarLock stands down when it is loaded (see TXCLTraderPlusNotice).
// Open the keypad. Shown to a NOT-yet-authorized player on a vehicle that has a lock
// (enter password), and to the owner with no password set yet (set password).

class ActionTXCLLockMenu : ActionInteractBase
{
    void ActionTXCLLockMenu()
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

        TXCLVehicleLockManager manager = TXCLVehicleLockManager.Get(target.GetObject());
        if (manager && !player.CanInteractWithTXCLLock(manager) && manager.HasVehicleLock())
            return true;

        return false;
    }

    override void OnStartClient(ActionData action_data)
    {
        TXCLVehicleLockManager manager = TXCLVehicleLockManager.Get(action_data.m_Target.GetObject());
        if (manager)
            GetTXCLModule().OpenLockMenu(manager);
    }
}
#endif
