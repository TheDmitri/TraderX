#ifdef CARCOVER
modded class CarCoverBase extends Container_Base
{
    override void StoreCarLock(CarScript car)
    {
        super.StoreCarLock(car);

        #ifdef TraderXCarLock
        if (car.m_CarLockOwner != TX_CL_UNDEFINED && car.m_CarLockOwner != 0)
        {
            m_CCCarLock_IsLocked = car.m_CarLock_IsLocked;
            m_CCCarLockOwner = car.m_CarLockOwner;
            m_CCCarLockPassword = car.m_CarLockPassword;
            m_CCSoundToPlay = car.m_LockTimestamp;
            GetTXCarLockLogger().LogInfo(string.Format("[CarCover] StoreCarLock - saved lock data: owner=%1, password=%2, isLocked=%3, timestamp=%4, lockId=%5_%4", m_CCCarLockOwner, m_CCCarLockPassword, m_CCCarLock_IsLocked, m_CCSoundToPlay, m_CCCarLockOwner));
        }
        else
        {
            GetTXCarLockLogger().LogInfo(string.Format("[CarCover] StoreCarLock - vehicle has no valid lock (owner=%1), skipping save", car.m_CarLockOwner));
        }
        #endif
    }

    override void RestoreCarLock(CarScript car)
    {
        super.RestoreCarLock(car);

        #ifdef TraderXCarLock
        int storedTimestamp = m_CCSoundToPlay;

        if (m_CCCarLockOwner == TX_CL_UNDEFINED || m_CCCarLockOwner == 0 || m_CCCarLockPassword == 0)
        {
            GetTXCarLockLogger().LogInfo(string.Format("[CarCover] RestoreCarLock - invalid lock data (owner=%1, password=%2), skipping restore", m_CCCarLockOwner, m_CCCarLockPassword));
            return;
        }

        VehicleLockManager manager = VehicleLockManager.Get(car);
        if (!manager)
        {
            GetTXCarLockLogger().LogError("[CarCover] RestoreCarLock - failed to get VehicleLockManager");
            return;
        }

        string coverLockId = m_CCCarLockOwner.ToString() + "_" + storedTimestamp.ToString();

        GetTXCarLockLogger().LogInfo(string.Format("[CarCover] RestoreCarLock - cover data: owner=%1, password=%2, isLocked=%3, timestamp=%4, coverLockId=%5", m_CCCarLockOwner, m_CCCarLockPassword, m_CCCarLock_IsLocked, storedTimestamp, coverLockId));

        if (!manager.HasVehicleLockPassword())
        {
            if (storedTimestamp > 0)
            {
                GetTXCarLockLogger().LogInfo(string.Format("[CarCover] RestoreCarLock - restoring with original timestamp: owner=%1, password=%2, isLocked=%3, timestamp=%4", m_CCCarLockOwner, m_CCCarLockPassword, m_CCCarLock_IsLocked, storedTimestamp));
                manager.RestoreVehicleLockData(m_CCCarLockOwner, m_CCCarLockPassword, m_CCCarLock_IsLocked, storedTimestamp);
            }
            else
            {
                GetTXCarLockLogger().LogWarning(string.Format("[CarCover] RestoreCarLock - no valid timestamp in cover, initializing with new timestamp: owner=%1, password=%2", m_CCCarLockOwner, m_CCCarLockPassword));
                manager.InitializeVehicleLockData(m_CCCarLockOwner, m_CCCarLockPassword);
                manager.SetVehicleLock(m_CCCarLock_IsLocked);
            }
            GetTXCarLockLogger().LogInfo(string.Format("[CarCover] RestoreCarLock - DONE - new lockId=%1", manager.GetLockId()));
        }
        else
        {
            GetTXCarLockLogger().LogInfo("[CarCover] RestoreCarLock - vehicle already has lock password, skipping init");
        }
        #endif
        car.SetSynchDirty();
    }

    override bool OnStoreLoad(ParamsReadContext ctx, int version)
    {
        if (!super.OnStoreLoad(ctx, version))
            return false;

        #ifdef TraderXCarLock
        if (!ctx.Read(m_CCCarLock_IsLocked))
            return false;
        if (!ctx.Read(m_CCCarLockOwner))
            return false;
        if (!ctx.Read(m_CCCarLockPassword))
            return false;
        if (!ctx.Read(m_CCSoundToPlay))
            return false;
        #endif

        return true;
    }

    override void OnStoreSave(ParamsWriteContext ctx)
    {
        super.OnStoreSave(ctx);

        #ifdef TraderXCarLock
        ctx.Write(m_CCCarLock_IsLocked);
        ctx.Write(m_CCCarLockOwner);
        ctx.Write(m_CCCarLockPassword);
        ctx.Write(m_CCSoundToPlay);
        #endif
    }
}
#endif
