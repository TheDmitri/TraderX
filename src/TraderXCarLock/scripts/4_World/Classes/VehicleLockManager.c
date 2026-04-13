class VehicleLockManager
{
    protected ref VehicleLockComponent m_LockComponent;

    void VehicleLockManager()
    {
    }

    void ~VehicleLockManager()
    {
    }

    static VehicleLockManager Get(IEntity entity, bool errorOnFailure = false)
    {
        CarScript car;
        BoatScript boat;

        if (Class.CastTo(car, entity))
        {
            return car.GetVehicleLockManager();
        }
        else if (Class.CastTo(boat, entity))
        {
            return boat.GetVehicleLockManager();
        }

        if (errorOnFailure)
            GetTXCarLockLogger().LogError(string.Format("%1 is not a lockable vehicle type", entity));

        return null;
    }

    static bool Get(out VehicleLockManager manager, IEntity entity, bool errorOnFailure = false)
    {
        manager = Get(entity, errorOnFailure);
        return manager != null;
    }

    EntityAI GetEntity()
    {
        GetTXCarLockLogger().LogError("GetEntity() NOT IMPLEMENTED");
        return null;
    }

    Transport GetTransport()
    {
        GetTXCarLockLogger().LogError("GetTransport() NOT IMPLEMENTED");
        return null;
    }

    void Init()
    {
        GetTXCarLockLogger().LogError("Init() NOT IMPLEMENTED");
    }

    void SyncFromComponent()
    {
        GetTXCarLockLogger().LogError("SyncFromComponent() NOT IMPLEMENTED");
    }

    void SyncToComponent()
    {
        GetTXCarLockLogger().LogError("SyncToComponent() NOT IMPLEMENTED");
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

    void SetVehicleLock(bool lock, int soundId = TX_CL_UNDEFINED)
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
        {
            GetTXCarLockLogger().LogError("GetLockId() m_LockComponent is null");
            return "";
        }

        return m_LockComponent.GetVehicleLockId();
    }

    int GetVehicleLockOwner()
    {
        if (m_LockComponent)
            return m_LockComponent.GetLockOwner();

        return TX_CL_UNDEFINED;
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

        return TX_CL_UNDEFINED;
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
            {
                TXCarLockPlayerDb.RemoveVehicleFromAllPlayers(lockId);
            }
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

    void SetSoundToPlay(int soundId = ETXCarLockSounds.NO_SOUND)
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

    void SaveData(ParamsWriteContext ctx)
    {
        if (m_LockComponent)
            m_LockComponent.SaveData(ctx);
    }

    bool LoadData(ParamsReadContext ctx)
    {
        if (m_LockComponent)
        {
            m_LockComponent.LoadData(ctx);
            SyncFromComponent();
        }
        return true;
    }

    void LogDelete()
    {
        if (m_LockComponent)
            m_LockComponent.LogDelete();
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

    string ToDebugString()
    {
        if (!m_LockComponent)
        {
            GetTXCarLockLogger().LogError("ToDebugString() m_LockComponent is null");
            return "";
        }

        if (!GetEntity())
        {
            GetTXCarLockLogger().LogError("ToDebugString() GetEntity() is null");
            return "";
        }

        string result = string.Format("VehicleLockManager[%1]:", GetEntity().GetType());
        result += string.Format(" HasLock=%1", HasVehicleLock());
        result += string.Format(" IsLocked=%1", IsVehicleLocked());
        result += string.Format(" Owner=%1", GetVehicleLockOwner());
        result += string.Format(" Password=%1", GetVehicleLockPassword());
        result += string.Format(" HasPassword=%1", HasVehicleLockPassword());
        result += string.Format(" HasPlayerInside=%1", HasPlayerInside());
        result += string.Format(" SoundToPlay=%1", m_LockComponent.GetSoundToPlay());

        return result;
    }
}

class VehicleLockManagerT<Class T>: VehicleLockManager
{
    protected T m_Vehicle;

    void VehicleLockManagerT(T vehicle)
    {
        m_Vehicle = vehicle;
        m_LockComponent = new VehicleLockComponent(vehicle);
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

class CarLockManager: VehicleLockManagerT<CarScript>
{
    void CarLockManager(CarScript vehicle)
    {
    }

    override void Init()
    {
        m_Vehicle.RegisterNetSyncVariableBool("m_CarLock_IsLocked");
        m_Vehicle.RegisterNetSyncVariableInt("m_CarLockOwner");
        m_Vehicle.RegisterNetSyncVariableInt("m_CarLockPassword");
        m_Vehicle.RegisterNetSyncVariableInt("m_LockTimestamp");
        m_Vehicle.RegisterNetSyncVariableInt("m_SoundToPlay");
    }

    override void SyncFromComponent()
    {
        if (m_LockComponent)
        {
            m_Vehicle.m_CarLock_IsLocked = m_LockComponent.GetIsLocked();
            m_Vehicle.m_CarLockOwner = m_LockComponent.GetLockOwner();
            m_Vehicle.m_CarLockPassword = m_LockComponent.GetLockPassword();
            m_Vehicle.m_LockTimestamp = m_LockComponent.GetLockTimestamp();
            m_Vehicle.m_SoundToPlay = m_LockComponent.GetSoundToPlay();
            m_Vehicle.SetSynchDirty();
        }
    }

    override void SyncToComponent()
    {
        if (m_LockComponent)
        {
            m_LockComponent.SetIsLocked(m_Vehicle.m_CarLock_IsLocked);
            m_LockComponent.SetLockOwner(m_Vehicle.m_CarLockOwner);
            m_LockComponent.SetLockPassword(m_Vehicle.m_CarLockPassword);
            m_LockComponent.SetLockTimestamp(m_Vehicle.m_LockTimestamp);
            m_LockComponent.SetSoundToPlayDirect(m_Vehicle.m_SoundToPlay);
        }
    }
}

class BoatLockManager: VehicleLockManagerT<BoatScript>
{
    void BoatLockManager(BoatScript vehicle)
    {
    }

    override void Init()
    {
        m_Vehicle.RegisterNetSyncVariableBool("m_CarLock_IsLocked");
        m_Vehicle.RegisterNetSyncVariableInt("m_CarLockOwner");
        m_Vehicle.RegisterNetSyncVariableInt("m_CarLockPassword");
        m_Vehicle.RegisterNetSyncVariableInt("m_LockTimestamp");
        m_Vehicle.RegisterNetSyncVariableInt("m_SoundToPlay");
    }

    override void SyncFromComponent()
    {
        if (m_LockComponent)
        {
            m_Vehicle.m_CarLock_IsLocked = m_LockComponent.GetIsLocked();
            m_Vehicle.m_CarLockOwner = m_LockComponent.GetLockOwner();
            m_Vehicle.m_CarLockPassword = m_LockComponent.GetLockPassword();
            m_Vehicle.m_LockTimestamp = m_LockComponent.GetLockTimestamp();
            m_Vehicle.m_SoundToPlay = m_LockComponent.GetSoundToPlay();
            m_Vehicle.SetSynchDirty();
        }
    }

    override void SyncToComponent()
    {
        if (m_LockComponent)
        {
            m_LockComponent.SetIsLocked(m_Vehicle.m_CarLock_IsLocked);
            m_LockComponent.SetLockOwner(m_Vehicle.m_CarLockOwner);
            m_LockComponent.SetLockPassword(m_Vehicle.m_CarLockPassword);
            m_LockComponent.SetLockTimestamp(m_Vehicle.m_LockTimestamp);
            m_LockComponent.SetSoundToPlayDirect(m_Vehicle.m_SoundToPlay);
        }
    }
}
