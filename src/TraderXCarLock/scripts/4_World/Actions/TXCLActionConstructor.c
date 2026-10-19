#ifndef CarLock
// TraderPlus V1 defines "CarLock" and owns the CarLock item: TraderX CarLock stands down when it is loaded (see TXCLTraderPlusNotice).
// Register the TXCL actions with the action system.

modded class ActionConstructor
{
    override void RegisterActions(TTypenameArray actions)
    {
        super.RegisterActions(actions);
        actions.Insert(ActionTXCLAttachLock);
        actions.Insert(ActionTXCLDetachLock);
        actions.Insert(ActionTXCLLockpick);
        actions.Insert(ActionTXCLLock);
        actions.Insert(ActionTXCLUnlock);
        actions.Insert(ActionTXCLLockMenu);
    }
}
#endif
