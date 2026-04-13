modded class BoatScript
{
    bool m_CarLock_IsLocked = false;
    int m_CarLockOwner = TX_CL_UNDEFINED;
    int m_CarLockPassword = TX_CL_UNDEFINED;
    int m_LockTimestamp = 0;
    int m_SoundToPlay = ETXCarLockSounds.NO_SOUND;

    ref VehicleLockManager m_VehicleLockManager;

    void BoatScript()
    {
        m_VehicleLockManager = new BoatLockManager(this);
        m_VehicleLockManager.Init();
    }

    override void EEDelete(EntityAI parent)
    {
        super.EEDelete(parent);
        if (GetGame().IsServer() && m_VehicleLockManager)
        {
            m_VehicleLockManager.LogDelete();
        }
    }

    VehicleLockManager GetVehicleLockManager()
    {
        return m_VehicleLockManager;
    }

    override void OnVariablesSynchronized()
    {
        super.OnVariablesSynchronized();
        m_VehicleLockManager.SyncToComponent();
        m_VehicleLockManager.HandleSoundSync();
    }

    override bool CanDisplayAttachmentSlot(string slot_name)
    {
        if (!super.CanDisplayAttachmentSlot(slot_name))
            return false;
        return !m_CarLock_IsLocked;
    }

    override bool CanDisplayAttachmentCategory(string category_name)
    {
        if (!super.CanDisplayAttachmentCategory(category_name))
            return false;
        return !m_CarLock_IsLocked;
    }

    override bool IsInventoryVisible()
    {
        if (!super.IsInventoryVisible())
            return false;
        return !m_CarLock_IsLocked && (GetGame().GetPlayer() && (!GetGame().GetPlayer().GetCommand_Vehicle() || GetGame().GetPlayer().GetCommand_Vehicle().GetTransport() == this));
    }

    override void OnStoreSave(ParamsWriteContext ctx)
    {
        super.OnStoreSave(ctx);
        if (m_VehicleLockManager)
            m_VehicleLockManager.SaveData(ctx);
    }

    override bool OnStoreLoad(ParamsReadContext ctx, int version)
    {
        if (!super.OnStoreLoad(ctx, version))
            return false;
        if (m_VehicleLockManager)
            return m_VehicleLockManager.LoadData(ctx);
        return true;
    }
}
