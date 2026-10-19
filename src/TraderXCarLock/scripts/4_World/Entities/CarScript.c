#ifndef CarLock
// TraderPlus V1 defines "CarLock" and owns the CarLock item: TraderX CarLock stands down when it is loaded (see TXCLTraderPlusNotice).
// Hooks the TXCL lock system into CarScript via composition. All added members/methods are
// TXCL-prefixed so this can coexist with other lock mods (TL_CarLock) modding CarScript too.

modded class CarScript
{
    bool m_TXCL_IsLocked = false;
    int  m_TXCL_Owner = TXCL_UNDEFINED;
    int  m_TXCL_Password = TXCL_UNDEFINED;
    int  m_TXCL_Timestamp = 0;
    int  m_TXCL_Sound = ETXCLCarLockSounds.NO_SOUND;

    ref TXCLVehicleLockManager m_TXCLLockManager;
    protected bool m_TXCL_LoadedFromModStorage;

    void CarScript()
    {
        m_TXCLLockManager = new TXCLCarLockManager(this);
        m_TXCLLockManager.Init();
    }

    TXCLVehicleLockManager GetTXCLLockManager()
    {
        return m_TXCLLockManager;
    }

    override void OnVariablesSynchronized()
    {
        super.OnVariablesSynchronized();
        if (m_TXCLLockManager)
        {
            m_TXCLLockManager.SyncToComponent();
            m_TXCLLockManager.HandleSoundSync();
        }
    }

    override bool CanDisplayAttachmentSlot(string slot_name)
    {
        if (!super.CanDisplayAttachmentSlot(slot_name))
            return false;
        return !m_TXCL_IsLocked;
    }

    override bool CanDisplayAttachmentCategory(string category_name)
    {
        if (!super.CanDisplayAttachmentCategory(category_name))
            return false;
        return !m_TXCL_IsLocked;
    }

    override bool IsInventoryVisible()
    {
        if (!super.IsInventoryVisible())
            return false;
        return !m_TXCL_IsLocked && (GetGame().GetPlayer() && (!GetGame().GetPlayer().GetCommand_Vehicle() || GetGame().GetPlayer().GetCommand_Vehicle().GetTransport() == this));
    }

    // Lock data is persisted through CF ModStorage (see TXCLStorageMode).
    override void CF_OnStoreSave(CF_ModStorageMap storage)
    {
        super.CF_OnStoreSave(storage);

        CF_ModStorage ctx = storage.Get(TXCL_STORAGE_MOD);
        if (ctx && m_TXCLLockManager)
            m_TXCLLockManager.SaveData(ctx);
    }

    override bool CF_OnStoreLoad(CF_ModStorageMap storage)
    {
        if (!super.CF_OnStoreLoad(storage))
            return false;

        CF_ModStorage ctx = storage.Get(TXCL_STORAGE_MOD);
        if (!ctx)
            return true;

        m_TXCL_LoadedFromModStorage = true;
        if (m_TXCLLockManager)
            m_TXCLLockManager.LoadData(ctx);

        // Never fail the vehicle load because of the lock - that would drop the vehicle.
        return true;
    }

    override bool OnStoreLoad(ParamsReadContext ctx, int version)
    {
        if (!super.OnStoreLoad(ctx, version))
            return false;

        // Saved by a pre-ModStorage TraderXCarLock build: the lock sits here as raw data.
        if (!m_TXCL_LoadedFromModStorage && m_TXCLLockManager && TXCLStorageMode.ReadsLegacyData())
            m_TXCLLockManager.LoadLegacyData(ctx);

        return true;
    }
}
#endif
