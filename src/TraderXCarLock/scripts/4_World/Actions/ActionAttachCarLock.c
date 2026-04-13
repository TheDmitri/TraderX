class ActionAttachCarLock: ActionSingleUseBase
{
    void ActionAttachCarLock()
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
        return "Attach CarLock";
    }

    override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
    {
        if (GetGame().IsServer())
            return true;

        if (!CF_String.EqualsIgnoreCase(ItemBase.GetTXCarLockClassName(), item.GetType()))
            return false;

        VehicleLockManager manager = VehicleLockManager.Get(target.GetObject());
        if (manager && !manager.HasPlayerInside() && !manager.HasVehicleLock())
            return true;

        return false;
    }

    override void OnExecuteClient(ActionData action_data)
    {
        VehicleLockManager manager = VehicleLockManager.Get(action_data.m_Target.GetObject());
        if (manager && action_data.m_MainItem && action_data.m_MainItem.GetHierarchyRootPlayer() == action_data.m_Player)
        {
            GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(OpenCarLockMenu, 100, false, manager);
        }
    }

    override void OnExecuteServer(ActionData action_data)
    {
        VehicleLockManager manager = VehicleLockManager.Get(action_data.m_Target.GetObject());
        TXCarLockPlayerDb playerDb = action_data.m_Player.GetCarLockDb();
        if (manager && action_data.m_MainItem && action_data.m_MainItem.GetHierarchyRootPlayer() == action_data.m_Player && playerDb)
        {
            action_data.m_MainItem.Delete();
            manager.InitializeVehicleLockData(playerDb.GetOwnerId(), TX_CL_UNDEFINED);
            string lockId = manager.GetLockId();
            if (lockId != "")
                playerDb.AddUnlockedVehicle(lockId);
        }
    }

    void OpenCarLockMenu(VehicleLockManager manager)
    {
        GetTXCarLockUIService().OpenCarLockMenu(manager);
    }
}
