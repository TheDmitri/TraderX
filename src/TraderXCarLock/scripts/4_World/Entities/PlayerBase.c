// Per-player carlock DB cache + authorization check + registers the player-held lock actions.

modded class PlayerBase
{
    private ref TXCLPlayerDb m_TXCLPlayerDb;

    override void Init()
    {
        super.Init();
        m_TXCLPlayerDb = new TXCLPlayerDb();
    }

    void SetTXCLPlayerDb(TXCLPlayerDb carLockPlayerDb)
    {
        m_TXCLPlayerDb = carLockPlayerDb;
    }

    TXCLPlayerDb GetTXCLPlayerDb()
    {
        return m_TXCLPlayerDb;
    }

    // Authorized = admin or has previously unlocked this exact lock (owner+timestamp id).
    bool CanInteractWithTXCLLock(TXCLVehicleLockManager manager)
    {
        if (!manager || !manager.HasVehicleLock())
            return false;

        if (!m_TXCLPlayerDb)
            return false;

        if (m_TXCLPlayerDb.IsAdmin())
            return true;

        if (m_TXCLPlayerDb.HasUnlockedVehicle(manager.GetLockId()))
            return true;

        return false;
    }

    override void SetActions()
    {
        super.SetActions();
        AddAction(ActionTXCLLock);
        AddAction(ActionTXCLUnlock);
        AddAction(ActionTXCLLockMenu);
        AddAction(ActionTXCLDetachLock);
    }
}
