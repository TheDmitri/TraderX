modded class PlayerBase extends ManBase
{
    private ref TXCarLockPlayerDb _carLockPlayerDb;

    override void Init()
    {
        super.Init();
        _carLockPlayerDb = new TXCarLockPlayerDb();
    }

    void SetTXCarLockPlayerDb(TXCarLockPlayerDb carLockPlayerDb)
    {
        _carLockPlayerDb = carLockPlayerDb;
    }

    TXCarLockPlayerDb GetCarLockDb()
    {
        return _carLockPlayerDb;
    }

    bool CanInteractWithVehicleLock(VehicleLockManager manager)
    {
        GetTXCarLockLogger().LogDebug("CanInteractWithVehicleLock");
        if (!manager.HasVehicleLock())
        {
            GetTXCarLockLogger().LogDebug("vehicle doesn't have a lock");
            return false;
        }

        if (_carLockPlayerDb.IsAdmin())
        {
            GetTXCarLockLogger().LogDebug("player is an admin");
            return true;
        }

        if (_carLockPlayerDb.HasUnlockedVehicle(manager.GetLockId()))
        {
            GetTXCarLockLogger().LogDebug("player has unlocked this vehicle");
            return true;
        }

        GetTXCarLockLogger().LogDebug("CanInteractWithVehicleLock => false");
        return false;
    }

    bool CanInteractWithCarLock(CarScript car)
    {
        VehicleLockManager manager = VehicleLockManager.Get(car);
        if (manager)
            return CanInteractWithVehicleLock(manager);
        return false;
    }

    bool CanInteractWithCarLock(BoatScript boat)
    {
        VehicleLockManager manager = VehicleLockManager.Get(boat);
        if (manager)
            return CanInteractWithVehicleLock(manager);
        return false;
    }

    override void SetActions()
    {
        super.SetActions();
        AddAction(ActionCarLock);
        AddAction(ActionCarUnlock);
        AddAction(ActionCarLockMenu);
        AddAction(ActionDetachCarLock);
    }
}
