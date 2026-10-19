#ifndef CarLock
// TraderPlus V1 defines "CarLock" and owns the CarLock item: TraderX CarLock stands down when it is loaded (see TXCLTraderPlusNotice).
// Composition wrapper around any Transport (Car/Boat). Owns a TXCLVehicleLockComponent and
// bridges it to the vehicle's net-sync variables. Resolved via TXCLVehicleLockManager.Get(entity).

class TXCLVehicleLockManager
{
    protected ref TXCLVehicleLockComponent m_LockComponent;

    void TXCLVehicleLockManager()
    {
    }

    void ~TXCLVehicleLockManager()
    {
    }

    static TXCLVehicleLockManager Get(IEntity entity)
    {
        CarScript car;
        BoatScript boat;

        if (Class.CastTo(car, entity))
            return car.GetTXCLLockManager();

        if (Class.CastTo(boat, entity))
            return boat.GetTXCLLockManager();

        return null;
    }

    static bool Get(out TXCLVehicleLockManager manager, IEntity entity)
    {
        manager = Get(entity);
        return manager != null;
    }

    // Implemented by the vehicle-specific subclasses.
    EntityAI GetEntity()
    {
        return null;
    }

    Transport GetTransport()
    {
        return null;
    }

    void Init()
    {
    }

    void SyncFromComponent()
    {
    }

    void SyncToComponent()
    {
    }

    bool HasVehicleLock()
    {
        if (m_LockComponent)
            return m_LockComponent.HasLock();
        return false;
    }

    bool IsVehicleLocked()
    {
        if (m_LockComponent)
            return m_LockComponent.GetIsLocked();
        return false;
    }

    void SetVehicleLock(bool lock, int soundId = TXCL_UNDEFINED)
    {
        if (m_LockComponent)
        {
            m_LockComponent.SetLock(lock, soundId);
            SyncFromComponent();
        }
    }

    string GetLockId()
    {
        if (!m_LockComponent)
            return "";
        return m_LockComponent.GetVehicleLockId();
    }

    int GetVehicleLockOwner()
    {
        if (m_LockComponent)
            return m_LockComponent.GetLockOwner();
        return TXCL_UNDEFINED;
    }

    void SetVehicleLockOwner(int ownerId)
    {
        if (m_LockComponent)
        {
            m_LockComponent.SetOwner(ownerId);
            SyncFromComponent();
        }
    }

    int GetVehicleLockPassword()
    {
        if (m_LockComponent)
            return m_LockComponent.GetLockPassword();
        return TXCL_UNDEFINED;
    }

    void SetVehicleLockPassword(int password)
    {
        if (m_LockComponent)
        {
            m_LockComponent.SetPassword(password);
            SyncFromComponent();
        }
    }

    bool HasVehicleLockPassword()
    {
        if (m_LockComponent)
            return m_LockComponent.HasPassword();
        return false;
    }

    void InitializeVehicleLockData(int ownerId, int password)
    {
        if (m_LockComponent)
        {
            m_LockComponent.InitializeLockData(ownerId, password);
            SyncFromComponent();
        }
    }

    void RestoreVehicleLockData(int ownerId, int password, bool isLocked, int timestamp)
    {
        if (m_LockComponent)
        {
            m_LockComponent.RestoreLockData(ownerId, password, isLocked, timestamp);
            SyncFromComponent();
        }
    }

    void ResetVehicleLockData()
    {
        if (m_LockComponent)
        {
            string lockId = GetLockId();
            if (lockId != "")
                TXCLPlayerDb.RemoveVehicleFromAllPlayers(lockId);
            m_LockComponent.ResetLockData();
            SyncFromComponent();
        }
    }

    bool HasPlayerInside()
    {
        if (m_LockComponent)
            return m_LockComponent.HasPlayerInside();
        return false;
    }

    void SynchronizeLocking()
    {
        if (m_LockComponent)
            m_LockComponent.SynchronizeLocking();
    }

    void SetSoundToPlay(int soundId = ETXCLCarLockSounds.NO_SOUND)
    {
        if (m_LockComponent)
        {
            m_LockComponent.SetSoundToPlay(soundId);
            SyncFromComponent();
        }
    }

    void HandleSoundSync()
    {
        if (m_LockComponent)
            m_LockComponent.HandleSoundSync();
    }

    void SaveData(CF_ModStorage ctx)
    {
        if (m_LockComponent)
            m_LockComponent.SaveData(ctx);
    }

    bool LoadData(CF_ModStorage ctx)
    {
        if (!m_LockComponent)
            return true;

        bool loaded = m_LockComponent.LoadData(ctx);
        SyncFromComponent();
        return loaded;
    }

    void LoadLegacyData(ParamsReadContext ctx)
    {
        if (!m_LockComponent)
            return;

        m_LockComponent.LoadLegacyData(ctx);
        SyncFromComponent();
    }

    void PlayLockSound()
    {
        if (m_LockComponent)
            m_LockComponent.PlayLockSound();
    }

    void PlayUnlockSound()
    {
        if (m_LockComponent)
            m_LockComponent.PlayUnlockSound();
    }

    void PlayAlarmSound()
    {
        if (m_LockComponent)
            m_LockComponent.PlayAlarmSound();
    }

    void StopAlarm()
    {
        if (m_LockComponent)
            m_LockComponent.StopAlarm();
    }
}

class TXCLVehicleLockManagerT<Class T> : TXCLVehicleLockManager
{
    protected T m_Vehicle;

    void TXCLVehicleLockManagerT(T vehicle)
    {
        m_Vehicle = vehicle;
        m_LockComponent = new TXCLVehicleLockComponent(vehicle);
    }

    override EntityAI GetEntity()
    {
        return m_Vehicle;
    }

    override Transport GetTransport()
    {
        return m_Vehicle;
    }
}

class TXCLCarLockManager : TXCLVehicleLockManagerT<CarScript>
{
    void TXCLCarLockManager(CarScript vehicle)
    {
    }

    override void Init()
    {
        m_Vehicle.RegisterNetSyncVariableBool("m_TXCL_IsLocked");
        m_Vehicle.RegisterNetSyncVariableInt("m_TXCL_Owner");
        m_Vehicle.RegisterNetSyncVariableInt("m_TXCL_Password");
        m_Vehicle.RegisterNetSyncVariableInt("m_TXCL_Timestamp");
        m_Vehicle.RegisterNetSyncVariableInt("m_TXCL_Sound");
    }

    override void SyncFromComponent()
    {
        if (m_LockComponent)
        {
            m_Vehicle.m_TXCL_IsLocked = m_LockComponent.GetIsLocked();
            m_Vehicle.m_TXCL_Owner = m_LockComponent.GetLockOwner();
            m_Vehicle.m_TXCL_Password = m_LockComponent.GetLockPassword();
            m_Vehicle.m_TXCL_Timestamp = m_LockComponent.GetLockTimestamp();
            m_Vehicle.m_TXCL_Sound = m_LockComponent.GetSoundToPlay();
            m_Vehicle.SetSynchDirty();
        }
    }

    override void SyncToComponent()
    {
        if (m_LockComponent)
        {
            m_LockComponent.SetIsLocked(m_Vehicle.m_TXCL_IsLocked);
            m_LockComponent.SetLockOwner(m_Vehicle.m_TXCL_Owner);
            m_LockComponent.SetLockPassword(m_Vehicle.m_TXCL_Password);
            m_LockComponent.SetLockTimestamp_NoSync(m_Vehicle.m_TXCL_Timestamp);
            m_LockComponent.SetSoundToPlayDirect(m_Vehicle.m_TXCL_Sound);
        }
    }
}

class TXCLBoatLockManager : TXCLVehicleLockManagerT<BoatScript>
{
    void TXCLBoatLockManager(BoatScript vehicle)
    {
    }

    override void Init()
    {
        m_Vehicle.RegisterNetSyncVariableBool("m_TXCL_IsLocked");
        m_Vehicle.RegisterNetSyncVariableInt("m_TXCL_Owner");
        m_Vehicle.RegisterNetSyncVariableInt("m_TXCL_Password");
        m_Vehicle.RegisterNetSyncVariableInt("m_TXCL_Timestamp");
        m_Vehicle.RegisterNetSyncVariableInt("m_TXCL_Sound");
    }

    override void SyncFromComponent()
    {
        if (m_LockComponent)
        {
            m_Vehicle.m_TXCL_IsLocked = m_LockComponent.GetIsLocked();
            m_Vehicle.m_TXCL_Owner = m_LockComponent.GetLockOwner();
            m_Vehicle.m_TXCL_Password = m_LockComponent.GetLockPassword();
            m_Vehicle.m_TXCL_Timestamp = m_LockComponent.GetLockTimestamp();
            m_Vehicle.m_TXCL_Sound = m_LockComponent.GetSoundToPlay();
            m_Vehicle.SetSynchDirty();
        }
    }

    override void SyncToComponent()
    {
        if (m_LockComponent)
        {
            m_LockComponent.SetIsLocked(m_Vehicle.m_TXCL_IsLocked);
            m_LockComponent.SetLockOwner(m_Vehicle.m_TXCL_Owner);
            m_LockComponent.SetLockPassword(m_Vehicle.m_TXCL_Password);
            m_LockComponent.SetLockTimestamp_NoSync(m_Vehicle.m_TXCL_Timestamp);
            m_LockComponent.SetSoundToPlayDirect(m_Vehicle.m_TXCL_Sound);
        }
    }
}
#endif
