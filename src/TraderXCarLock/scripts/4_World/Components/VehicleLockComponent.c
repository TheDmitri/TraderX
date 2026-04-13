class VehicleLockComponent
{
    protected Transport m_Vehicle;
    protected bool m_IsLocked = false;
    protected int m_LockOwner = TX_CL_UNDEFINED;
    protected int m_LockPassword = TX_CL_UNDEFINED;
    protected int m_LockTimestamp = 0;
    protected int m_SoundToPlay = ETXCarLockSounds.NO_SOUND;
    protected int m_ClientLastSoundPlayed = ETXCarLockSounds.NO_SOUND;

    protected EffectSound m_AlarmSound;
    protected EffectSound m_BipSound;

    void VehicleLockComponent(Transport vehicle)
    {
        m_Vehicle = vehicle;
    }

    void ~VehicleLockComponent()
    {
        StopAlarm();
        GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(ResetSound);
    }

    string GetVehicleLockId()
    {
        return m_LockOwner.ToString() + "_" + m_LockTimestamp.ToString();
    }

    bool IsLocked()
    {
        return m_IsLocked;
    }

    bool HasLock()
    {
        return m_LockOwner != TX_CL_UNDEFINED;
    }

    bool HasPassword()
    {
        return m_LockPassword != TX_CL_UNDEFINED;
    }

    int GetOwner()
    {
        return m_LockOwner;
    }

    int GetPassword()
    {
        return m_LockPassword;
    }

    void SetOwner(int ownerId)
    {
        m_LockOwner = ownerId;
        if (m_Vehicle)
            m_Vehicle.SetSynchDirty();
    }

    void SetLockTimestamp(int timestamp)
    {
        m_LockTimestamp = timestamp;
        if (m_Vehicle)
            m_Vehicle.SetSynchDirty();
    }

    void SetPassword(int password)
    {
        m_LockPassword = password;
        if (m_Vehicle)
            m_Vehicle.SetSynchDirty();
    }

    void SetLock(bool lock, int soundId = TX_CL_UNDEFINED)
    {
        m_IsLocked = lock;
        SynchronizeLocking();

        if (soundId != TX_CL_UNDEFINED)
        {
            SetSoundToPlay(soundId);
        }
        else if (m_IsLocked)
        {
            SetSoundToPlay(ETXCarLockSounds.LOCK);
        }
        else
        {
            SetSoundToPlay(ETXCarLockSounds.UNLOCK);
        }
    }

    void InitializeLockData(int ownerId, int password)
    {
        SetOwner(ownerId);
        SetPassword(password);
        SetLock(true, ETXCarLockSounds.NO_SOUND);
        SetLockTimestamp(CF_Date.Now().GetTimestamp());
        LogClaimed();
    }

    void RestoreLockData(int ownerId, int password, bool isLocked, int timestamp)
    {
        SetOwner(ownerId);
        SetPassword(password);
        SetLock(isLocked, ETXCarLockSounds.NO_SOUND);
        SetLockTimestamp(timestamp);
        GetTXCarLockLogger().LogInfo(string.Format("RestoreLockData: owner=%1 password=%2 isLocked=%3 timestamp=%4 lockId=%5", ownerId, password, isLocked, timestamp, GetVehicleLockId()));
    }

    void ResetLockData()
    {
        LogUnClaimed();
        SetOwner(TX_CL_UNDEFINED);
        SetPassword(TX_CL_UNDEFINED);
        SetLock(false, ETXCarLockSounds.NO_SOUND);
    }

    bool HasPlayerInside()
    {
        if (!m_Vehicle)
            return false;

        int crewSize = m_Vehicle.CrewSize();
        for (int i = 0; i < crewSize; i++)
        {
            if (m_Vehicle.CrewMember(i))
                return true;
        }

        return false;
    }

    void SynchronizeLocking()
    {
        if (!GetGame().IsServer() || !m_Vehicle)
        {
            GetTXCarLockLogger().LogError("SynchronizeLocking() not server or m_Vehicle is null");
            return;
        }

        if (!m_Vehicle.GetInventory())
        {
            GetTXCarLockLogger().LogError("SynchronizeLocking() m_Vehicle.GetInventory() is null");
            return;
        }

        if (m_IsLocked)
        {
            m_Vehicle.GetInventory().LockInventory(HIDE_INV_FROM_SCRIPT);
        }
        else
        {
            m_Vehicle.GetInventory().UnlockInventory(HIDE_INV_FROM_SCRIPT);
        }

        m_Vehicle.SetSynchDirty();
    }

    int GetLastSoundPlayed()
    {
        if (GetGame().IsServer())
            return (m_SoundToPlay >> 8) & 0xFF;
        return m_ClientLastSoundPlayed;
    }

    void SetSoundToPlay(int soundId = ETXCarLockSounds.NO_SOUND)
    {
        m_SoundToPlay = soundId | (GetLastSoundPlayed() << 8);
        if (m_Vehicle)
            m_Vehicle.SetSynchDirty();

        if (soundId != ETXCarLockSounds.NO_SOUND)
            GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(ResetSound, 1000, false);
    }

    void ResetSound()
    {
        m_SoundToPlay = ETXCarLockSounds.NO_SOUND | (GetLastSoundPlayed() << 8);
        if (m_Vehicle)
            m_Vehicle.SetSynchDirty();
    }

    void HandleSoundSync()
    {
        int currentSound = m_SoundToPlay & 0xFF;

        if (currentSound == m_ClientLastSoundPlayed)
            return;

        m_ClientLastSoundPlayed = currentSound;

        switch (currentSound)
        {
            case ETXCarLockSounds.NO_SOUND:
                return;

            case ETXCarLockSounds.LOCK:
                PlayLockSound();
                break;

            case ETXCarLockSounds.UNLOCK:
                PlayUnlockSound();
                break;

            case ETXCarLockSounds.ALARM:
                PlayAlarmSound();
                break;
        }
    }

    void PlayLockSound()
    {
        StopAlarm();
        if (m_Vehicle)
            m_Vehicle.PlaySoundSet(m_BipSound, "TXcl_CarLock_SoundSet", 0, 0);
    }

    void PlayUnlockSound()
    {
        StopAlarm();
        if (m_Vehicle)
            m_Vehicle.PlaySoundSet(m_BipSound, "TXcl_CarUnlock_SoundSet", 0, 0);
    }

    void PlayAlarmSound()
    {
        StopAlarm();
        if (m_Vehicle)
        {
            m_Vehicle.PlaySoundSetLoop(m_AlarmSound, "TXcl_CarAlarm_SoundSet", 0, 0);
            GetTXCarLockLogger().LogInfo(string.Format("PlayAlarmSound: Alarm started on %1 at %2, auto-stop in 60 seconds", m_Vehicle.GetDisplayName(), m_Vehicle.GetPosition().ToString()));
            GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(StopAlarmWithLog, 60000, false);
        }
    }

    void StopAlarm()
    {
        if (m_AlarmSound && m_AlarmSound.IsSoundPlaying())
        {
            m_AlarmSound.SoundStop();
            if (m_Vehicle)
                GetTXCarLockLogger().LogInfo(string.Format("StopAlarm: Alarm stopped on %1 at %2", m_Vehicle.GetDisplayName(), m_Vehicle.GetPosition().ToString()));
        }
    }

    void StopAlarmWithLog()
    {
        if (m_Vehicle)
            GetTXCarLockLogger().LogInfo(string.Format("StopAlarmWithLog: Auto-stop timer fired for %1 at %2", m_Vehicle.GetDisplayName(), m_Vehicle.GetPosition().ToString()));
        StopAlarm();
    }

    void LogInit()
    {
        if (m_Vehicle)
            GetTXCarLockLogger().LogInfo("VehicleLockLogInit: " + m_Vehicle.GetDisplayName() + " position:" + m_Vehicle.GetPosition().ToString() + " ownerId:" + m_LockOwner + " isLocked:" + m_IsLocked);
    }

    void LogClaimed()
    {
        if (m_Vehicle)
            GetTXCarLockLogger().LogInfo("VehicleLockLogClaimed: " + m_Vehicle.GetDisplayName() + " position:" + m_Vehicle.GetPosition().ToString() + " ownerId:" + m_LockOwner + " isLocked:" + m_IsLocked);
    }

    void LogUnClaimed()
    {
        if (m_Vehicle)
            GetTXCarLockLogger().LogInfo("VehicleLockLogUnClaimed: " + m_Vehicle.GetDisplayName() + " position:" + m_Vehicle.GetPosition().ToString() + " ownerId:" + m_LockOwner + " isLocked:" + m_IsLocked);
    }

    void LogDelete()
    {
        if (m_Vehicle)
            GetTXCarLockLogger().LogInfo("VehicleLockLogDelete: " + m_Vehicle.GetDisplayName() + " position:" + m_Vehicle.GetPosition().ToString() + " ownerId:" + m_LockOwner + " isLocked:" + m_IsLocked);
    }

    void SaveData(ParamsWriteContext ctx)
    {
        GetTXCarLockLogger().LogInfo(string.Format("SaveData: Saving values - isLocked:%1 owner:%2 password:%3 timestamp:%4", m_IsLocked, m_LockOwner, m_LockPassword, m_LockTimestamp));

        Param4<bool, int, int, int> data = new Param4<bool, int, int, int>(m_IsLocked, m_LockOwner, m_LockPassword, m_LockTimestamp);
        ctx.Write(data);
    }

    bool LoadData(ParamsReadContext ctx)
    {
        Param4<bool, int, int, int> data = new Param4<bool, int, int, int>(false, TX_CL_UNDEFINED, TX_CL_UNDEFINED, TX_CL_UNDEFINED);
        if (ctx.Read(data))
        {
            m_IsLocked = data.param1;
            m_LockOwner = data.param2;
            m_LockPassword = data.param3;
            m_LockTimestamp = data.param4;

            GetTXCarLockLogger().LogInfo(string.Format("LoadData: Read values - isLocked:%1 owner:%2 password:%3 timestamp:%4", m_IsLocked, m_LockOwner, m_LockPassword, m_LockTimestamp));

            if (m_LockOwner == TX_CL_UNDEFINED || m_LockOwner == 0)
            {
                if (m_LockOwner == 0)
                    GetTXCarLockLogger().LogWarning(string.Format("LoadData: Corrupted lock data detected (owner=0, password=%1), resetting to undefined", m_LockPassword));
                m_IsLocked = false;
                m_LockOwner = TX_CL_UNDEFINED;
                m_LockPassword = TX_CL_UNDEFINED;
                m_LockTimestamp = 0;
                GetTXCarLockLogger().LogInfo("LoadData: Vehicle had no valid lock owner, forcing unlocked state");
            }

            LogInit();
            SynchronizeLocking();
        }
        else
        {
            GetTXCarLockLogger().LogDebug("LoadData: No lock data found in storage (vehicle has no lock)");
        }
        return true;
    }

    // Getters for network sync
    bool GetIsLocked() { return m_IsLocked; }
    int GetLockOwner() { return m_LockOwner; }
    int GetLockPassword() { return m_LockPassword; }
    int GetLockTimestamp() { return m_LockTimestamp; }
    int GetSoundToPlay() { return m_SoundToPlay & 0xFF; }

    // Setters for network sync
    void SetIsLocked(bool locked) { m_IsLocked = locked; }
    void SetLockOwner(int owner) { m_LockOwner = owner; }
    void SetLockPassword(int password) { m_LockPassword = password; }
    void SetSoundToPlayDirect(int sound) { m_SoundToPlay = sound | ((sound & 0xFF) << 8); }
}
