class ActionLockpickVehicleCB : ActionContinuousBaseCB
{
    private float m_LockpickTime;

    override void CreateActionComponent()
    {
        m_LockpickTime = 60.0;

        if (m_ActionData && m_ActionData.m_Target)
        {
            TXVehicleLockpickConfig config = VehicleTypeHelper.GetLockpickConfigForVehicle(m_ActionData.m_Target.GetObject());
            if (config)
                m_LockpickTime = config.lockpickTime;
        }

        m_ActionData.m_ActionComponent = new CAContinuousTime(m_LockpickTime);
    }
}

class ActionLockpickVehicle: ActionContinuousBase
{
    void ActionLockpickVehicle()
    {
        m_CallbackClass = ActionLockpickVehicleCB;
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

    override void OnStartServer(ActionData action_data)
    {
        VehicleLockManager manager = VehicleLockManager.Get(action_data.m_Target.GetObject());
        if (manager && action_data.m_MainItem && action_data.m_MainItem.GetHierarchyRootPlayer() == action_data.m_Player)
        {
            manager.SetSoundToPlay(ETXCarLockSounds.ALARM);

            EVehicleLockType vehicleType = VehicleTypeHelper.GetVehicleType(action_data.m_Target.GetObject());
            string vehicleTypeName = VehicleTypeHelper.GetVehicleTypeName(vehicleType);

            TXVehicleLockpickConfig config = VehicleTypeHelper.GetLockpickConfigForVehicle(action_data.m_Target.GetObject());
            if (config)
            {
                GetTXCarLockLogger().LogInfo(string.Format("Lockpicking config for %1: lockpickClassName=%2 lockpickTime=%3 lockpickChance=%4 lockpickDamagePerAttempt=%5 consumeOnSuccess=%6 consumeOnFailure=%7", vehicleTypeName, config.lockpickClassName, config.lockpickTime, config.lockpickChance, config.lockpickDamagePerAttempt, config.consumeLockpickOnSuccess, config.consumeLockpickOnFailure));
            }

            GetTXCarLockLogger().LogInfo(string.Format("Lockpicking started on %1 by player: %2 steamId: %3 position: %4", vehicleTypeName, action_data.m_Player.GetIdentity().GetName(), action_data.m_Player.GetCarLockDb().GetSteamId(), action_data.m_Player.GetPosition().ToString()));
        }
    }

    override void OnFinishProgressServer(ActionData action_data)
    {
        GetTXCarLockLogger().LogInfo("ActionLockpickVehicle: OnFinishProgressServer called");

        VehicleLockManager manager = VehicleLockManager.Get(action_data.m_Target.GetObject());
        if (!manager || !action_data.m_MainItem || action_data.m_MainItem.GetHierarchyRootPlayer() != action_data.m_Player)
            return;

        TXVehicleLockpickConfig config = VehicleTypeHelper.GetLockpickConfigForVehicle(action_data.m_Target.GetObject());
        if (!config)
        {
            GetTXCarLockLogger().LogError("ActionLockpickVehicle: Failed to get lockpick config");
            return;
        }

        float randomRoll = Math.RandomFloatInclusive(0, 1);
        bool success = randomRoll < config.lockpickChance;

        EVehicleLockType vehicleType = VehicleTypeHelper.GetVehicleType(action_data.m_Target.GetObject());
        string vehicleTypeName = VehicleTypeHelper.GetVehicleTypeName(vehicleType);

        if (success)
        {
            GetTXCarLockLogger().LogInfo(string.Format("Lockpicking SUCCESS on %1 (chance: %2, roll: %3) by player: %4 steamId: %5 position: %6 ownerId: %7", vehicleTypeName, config.lockpickChance, randomRoll, action_data.m_Player.GetIdentity().GetName(), action_data.m_Player.GetCarLockDb().GetSteamId(), action_data.m_Player.GetPosition().ToString(), manager.GetVehicleLockOwner()));

            manager.ResetVehicleLockData();
            manager.SetSoundToPlay(ETXCarLockSounds.NO_SOUND);

            if (config.consumeLockpickOnSuccess)
            {
                GetGame().ObjectDelete(action_data.m_MainItem);
                GetTXCarLockLogger().LogInfo("Lockpick consumed on success");
            }
            else
            {
                action_data.m_MainItem.DecreaseHealthCoef(config.lockpickDamagePerAttempt);
            }
        }
        else
        {
            GetTXCarLockLogger().LogInfo(string.Format("Lockpicking FAILED on %1 (chance: %2, roll: %3) by player: %4 steamId: %5 position: %6", vehicleTypeName, config.lockpickChance, randomRoll, action_data.m_Player.GetIdentity().GetName(), action_data.m_Player.GetCarLockDb().GetSteamId(), action_data.m_Player.GetPosition().ToString()));

            if (config.consumeLockpickOnFailure)
            {
                GetGame().ObjectDelete(action_data.m_MainItem);
                GetTXCarLockLogger().LogInfo("Lockpick consumed on failure");
            }
            else
            {
                action_data.m_MainItem.DecreaseHealthCoef(config.lockpickDamagePerAttempt);
            }
        }
    }

    override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
    {
        if (GetGame().IsServer())
            return true;

        TXVehicleLockpickConfig config = VehicleTypeHelper.GetLockpickConfigForVehicle(target.GetObject());
        if (!config)
            return false;

        if (!CF_String.EqualsIgnoreCase(config.lockpickClassName, item.GetType()))
            return false;

        VehicleLockManager manager = VehicleLockManager.Get(target.GetObject());
        if (manager && !manager.HasPlayerInside() && manager.HasVehicleLock() && manager.IsVehicleLocked())
            return true;

        return false;
    }
}
