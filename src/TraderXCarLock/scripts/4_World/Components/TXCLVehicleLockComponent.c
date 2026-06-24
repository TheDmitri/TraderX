// Holds the actual lock state for one vehicle (owner / password / locked / timestamp / sound).
// Composition target of TXCLVehicleLockManager - never inherits from the vehicle.

class TXCLVehicleLockComponent
{
    protected Transport m_Vehicle;
    protected bool m_IsLocked = false;
    protected int  m_LockOwner = TXCL_UNDEFINED;
    protected int  m_LockPassword = TXCL_UNDEFINED;
    protected int  m_LockTimestamp = 0;
    protected int  m_SoundToPlay = ETXCLCarLockSounds.NO_SOUND;
    protected int  m_ClientLastSoundPlayed = ETXCLCarLockSounds.NO_SOUND;

    protected EffectSound m_AlarmSound;
    protected EffectSound m_BipSound;

    void TXCLVehicleLockComponent(Transport vehicle)
    {
        m_Vehicle = vehicle;
    }

    void ~TXCLVehicleLockComponent()
    {
        StopAlarm();
        GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(ResetSound);
    }

    // Unique per claim: owner + first-lock timestamp. Used as the player-DB key.
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
        return m_LockOwner != TXCL_UNDEFINED;
    }

    bool HasPassword()
    {
        return m_LockPassword != TXCL_UNDEFINED;
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

    void SetLock(bool lock, int soundId = TXCL_UNDEFINED)
    {
        m_IsLocked = lock;
        SynchronizeLocking();

        if (soundId != TXCL_UNDEFINED)
            SetSoundToPlay(soundId);
        else if (m_IsLocked)
            SetSoundToPlay(ETXCLCarLockSounds.LOCK);
        else
            SetSoundToPlay(ETXCLCarLockSounds.UNLOCK);
    }

    void InitializeLockData(int ownerId, int password)
    {
        SetOwner(ownerId);
        SetPassword(password);
        SetLock(true, ETXCLCarLockSounds.NO_SOUND);
        SetLockTimestamp(CF_Date.Now().GetTimestamp());
    }

    void RestoreLockData(int ownerId, int password, bool isLocked, int timestamp)
    {
        SetOwner(ownerId);
        SetPassword(password);
        SetLock(isLocked, ETXCLCarLockSounds.NO_SOUND);
        SetLockTimestamp(timestamp);
    }

    void ResetLockData()
    {
        SetOwner(TXCL_UNDEFINED);
        SetPassword(TXCL_UNDEFINED);
        SetLock(false, ETXCLCarLockSounds.NO_SOUND);
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
            return;

        if (!m_Vehicle.GetInventory())
            return;

        if (m_IsLocked)
            m_Vehicle.GetInventory().LockInventory(HIDE_INV_FROM_SCRIPT);
        else
            m_Vehicle.GetInventory().UnlockInventory(HIDE_INV_FROM_SCRIPT);

        m_Vehicle.SetSynchDirty();
    }

    int GetLastSoundPlayed()
    {
        if (GetGame().IsServer())
            return (m_SoundToPlay >> 8) & 0xFF;
        return m_ClientLastSoundPlayed;
    }

    void SetSoundToPlay(int soundId = ETXCLCarLockSounds.NO_SOUND)
    {
        m_SoundToPlay = soundId | (GetLastSoundPlayed() << 8);
        if (m_Vehicle)
            m_Vehicle.SetSynchDirty();

        if (soundId != ETXCLCarLockSounds.NO_SOUND)
            GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(ResetSound, 1000, false);
    }

    void ResetSound()
    {
        m_SoundToPlay = ETXCLCarLockSounds.NO_SOUND | (GetLastSoundPlayed() << 8);
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
            case ETXCLCarLockSounds.NO_SOUND:
                return;

            case ETXCLCarLockSounds.LOCK:
                PlayLockSound();
                break;

            case ETXCLCarLockSounds.UNLOCK:
                PlayUnlockSound();
                break;

            case ETXCLCarLockSounds.ALARM:
                PlayAlarmSound();
                break;
        }
    }

    void PlayLockSound()
    {
        StopAlarm();
        if (m_Vehicle)
            m_Vehicle.PlaySoundSet(m_BipSound, "TXCL_CarLock_SoundSet", 0, 0);
    }

    void PlayUnlockSound()
    {
        StopAlarm();
        if (m_Vehicle)
            m_Vehicle.PlaySoundSet(m_BipSound, "TXCL_CarUnlock_SoundSet", 0, 0);
    }

    void PlayAlarmSound()
    {
        StopAlarm();
        if (m_Vehicle)
        {
            m_Vehicle.PlaySoundSetLoop(m_AlarmSound, "TXCL_CarAlarm_SoundSet", 0, 0);
            GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(StopAlarm, 60000, false);
        }
    }

    void StopAlarm()
    {
        if (m_AlarmSound && m_AlarmSound.IsSoundPlaying())
            m_AlarmSound.SoundStop();
    }

    void SaveData(ParamsWriteContext ctx)
    {
        Param4<bool, int, int, int> data = new Param4<bool, int, int, int>(m_IsLocked, m_LockOwner, m_LockPassword, m_LockTimestamp);
        ctx.Write(data);
    }

    bool LoadData(ParamsReadContext ctx)
    {
        Param4<bool, int, int, int> data = new Param4<bool, int, int, int>(false, TXCL_UNDEFINED, TXCL_UNDEFINED, TXCL_UNDEFINED);
        if (ctx.Read(data))
        {
            m_IsLocked = data.param1;
            m_LockOwner = data.param2;
            m_LockPassword = data.param3;
            m_LockTimestamp = data.param4;

            // No valid owner -> there is no lock; force a clean unlocked state.
            if (m_LockOwner == TXCL_UNDEFINED || m_LockOwner == 0)
            {
                m_IsLocked = false;
                m_LockOwner = TXCL_UNDEFINED;
                m_LockPassword = TXCL_UNDEFINED;
                m_LockTimestamp = 0;
            }

            SynchronizeLocking();
        }
        // Always true - a false return from OnStoreLoad can corrupt other mods' data.
        return true;
    }

    // Getters for the vehicle net-sync mirror.
    bool GetIsLocked() { return m_IsLocked; }
    int  GetLockOwner() { return m_LockOwner; }
    int  GetLockPassword() { return m_LockPassword; }
    int  GetLockTimestamp() { return m_LockTimestamp; }
    int  GetSoundToPlay() { return m_SoundToPlay & 0xFF; }

    // Setters used when syncing client net vars back into the component.
    void SetIsLocked(bool locked) { m_IsLocked = locked; }
    void SetLockOwner(int owner) { m_LockOwner = owner; }
    void SetLockPassword(int password) { m_LockPassword = password; }
    void SetLockTimestamp_NoSync(int timestamp) { m_LockTimestamp = timestamp; }
    void SetSoundToPlayDirect(int sound) { m_SoundToPlay = sound | ((sound & 0xFF) << 8); }
}
