// Lockpick a locked vehicle. Continuous, full-body, config-driven time. Triggers the alarm on
// start; on finish rolls the configured chance to break the lock open.

class ActionTXCLLockpickCB : ActionContinuousBaseCB
{
    private float m_LockpickTime;

    override void CreateActionComponent()
    {
        m_LockpickTime = 60.0;

        if (m_ActionData && m_ActionData.m_Target)
        {
            TXCLVehicleLockpickConfig config = TXCLVehicleTypeHelper.GetLockpickConfigForVehicle(m_ActionData.m_Target.GetObject());
            if (config)
                m_LockpickTime = config.lockpickTime;
        }

        m_ActionData.m_ActionComponent = new CAContinuousTime(m_LockpickTime);
    }
}

class ActionTXCLLockpick : ActionContinuousBase
{
    void ActionTXCLLockpick()
    {
        m_CallbackClass = ActionTXCLLockpickCB;
        m_CommandUID = DayZPlayerConstants.CMD_ACTIONFB_INTERACT;
        m_FullBody = true;
        m_StanceMask = DayZPlayerConstants.STANCEMASK_ERECT;
        m_SpecialtyWeight = UASoftSkillsWeight.PRECISE_LOW;
    }

    override void CreateConditionComponents()
    {
        m_ConditionTarget = new CCTNonRuined(10);
        m_ConditionItem = new CCINonRuined;
    }

    override string GetText()
    {
        return "Lockpick Vehicle";
    }

    override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
    {
        if (GetGame().IsServer())
            return true;

        TXCLVehicleLockpickConfig config = TXCLVehicleTypeHelper.GetLockpickConfigForVehicle(target.GetObject());
        if (!config)
            return false;

        if (!CF_String.EqualsIgnoreCase(config.lockpickClassName, item.GetType()))
            return false;

        TXCLVehicleLockManager manager = TXCLVehicleLockManager.Get(target.GetObject());
        if (manager && !manager.HasPlayerInside() && manager.HasVehicleLock() && manager.IsVehicleLocked())
            return true;

        return false;
    }

    override void OnStartServer(ActionData action_data)
    {
        TXCLVehicleLockManager manager = TXCLVehicleLockManager.Get(action_data.m_Target.GetObject());
        if (manager && action_data.m_MainItem && action_data.m_MainItem.GetHierarchyRootPlayer() == action_data.m_Player)
            manager.SetSoundToPlay(ETXCLCarLockSounds.ALARM);
    }

    override void OnFinishProgressServer(ActionData action_data)
    {
        TXCLVehicleLockManager manager = TXCLVehicleLockManager.Get(action_data.m_Target.GetObject());
        if (!manager || !action_data.m_MainItem || action_data.m_MainItem.GetHierarchyRootPlayer() != action_data.m_Player)
            return;

        TXCLVehicleLockpickConfig config = TXCLVehicleTypeHelper.GetLockpickConfigForVehicle(action_data.m_Target.GetObject());
        if (!config)
            return;

        float randomRoll = Math.RandomFloatInclusive(0, 1);
        bool success = randomRoll < config.lockpickChance;

        GetTXCLLog().LogInfo("[TXCL] Lockpick " + success + " by " + action_data.m_Player.GetIdentity().GetName() + " chance=" + config.lockpickChance + " roll=" + randomRoll);

        if (success)
        {
            manager.ResetVehicleLockData();
            manager.SetSoundToPlay(ETXCLCarLockSounds.NO_SOUND);

            if (config.consumeLockpickOnSuccess)
                GetGame().ObjectDelete(action_data.m_MainItem);
            else
                action_data.m_MainItem.DecreaseHealthCoef(config.lockpickDamagePerAttempt);
        }
        else
        {
            if (config.consumeLockpickOnFailure)
                GetGame().ObjectDelete(action_data.m_MainItem);
            else
                action_data.m_MainItem.DecreaseHealthCoef(config.lockpickDamagePerAttempt);
        }
    }
}
