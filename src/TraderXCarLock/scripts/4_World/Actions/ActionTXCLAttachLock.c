#ifndef CarLock
// TraderPlus V1 defines "CarLock" and owns the CarLock item: TraderX CarLock stands down when it is loaded (see TXCLTraderPlusNotice).
// Attach a lock item to an unlocked vehicle. The attacher becomes the owner (no password yet);
// they then set the password through the keypad (Open Lock Menu shows for non-authorized owner).

class ActionTXCLAttachLock : ActionSingleUseBase
{
    void ActionTXCLAttachLock()
    {
        m_CommandUID = DayZPlayerConstants.CMD_ACTIONMOD_ATTACHITEM;
        m_StanceMask = DayZPlayerConstants.STANCEMASK_ERECT | DayZPlayerConstants.STANCEMASK_CROUCH;
    }

    override void CreateConditionComponents()
    {
        m_ConditionItem = new CCINonRuined();
        m_ConditionTarget = new CCTNonRuined(UAMaxDistances.DEFAULT);
    }

    override string GetText()
    {
        return "Attach Lock";
    }

    override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
    {
        if (GetGame().IsServer())
            return true;

        if (!CF_String.EqualsIgnoreCase(ItemBase.GetTXCLLockClassName(), item.GetType()))
            return false;

        TXCLVehicleLockManager manager = TXCLVehicleLockManager.Get(target.GetObject());
        if (manager && !manager.HasPlayerInside() && !manager.HasVehicleLock())
            return true;

        return false;
    }

    override void OnExecuteServer(ActionData action_data)
    {
        TXCLVehicleLockManager manager = TXCLVehicleLockManager.Get(action_data.m_Target.GetObject());
        TXCLPlayerDb playerDb = action_data.m_Player.GetTXCLPlayerDb();
        if (manager && action_data.m_MainItem && action_data.m_MainItem.GetHierarchyRootPlayer() == action_data.m_Player && playerDb)
        {
            action_data.m_MainItem.Delete();
            // Owner set, no password. Authorization is granted only once a password is set,
            // so the "Open Lock Menu" (set-password UI) stays available to the owner.
            manager.InitializeVehicleLockData(playerDb.GetOwnerId(), TXCL_UNDEFINED);
        }
    }

    override void OnExecuteClient(ActionData action_data)
    {
        TXCLVehicleLockManager manager = TXCLVehicleLockManager.Get(action_data.m_Target.GetObject());
        if (manager && action_data.m_MainItem && action_data.m_MainItem.GetHierarchyRootPlayer() == action_data.m_Player)
            GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(OpenLockMenu, 100, false, manager);
    }

    void OpenLockMenu(TXCLVehicleLockManager manager)
    {
        GetTXCLModule().OpenLockMenu(manager);
    }
}
#endif
