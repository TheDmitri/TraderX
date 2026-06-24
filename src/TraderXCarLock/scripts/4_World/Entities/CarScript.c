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

    override void OnStoreSave(ParamsWriteContext ctx)
    {
        super.OnStoreSave(ctx);
        if (m_TXCLLockManager)
            m_TXCLLockManager.SaveData(ctx);
    }

    override bool OnStoreLoad(ParamsReadContext ctx, int version)
    {
        if (!super.OnStoreLoad(ctx, version))
            return false;
        if (m_TXCLLockManager)
            return m_TXCLLockManager.LoadData(ctx);
        return true;
    }
}
