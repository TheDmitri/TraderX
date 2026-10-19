#ifndef CarLock
// TraderPlus V1 defines "CarLock" and owns the CarLock item: TraderX CarLock stands down when it is loaded (see TXCLTraderPlusNotice).
// Holds the configured lock / lockpick class names and registers the item-held lock actions.

modded class ItemBase
{
    private static string m_TXCLLockClassName;
    private static string m_TXCLLockPickClassName;

    static string GetTXCLLockClassName()
    {
        return m_TXCLLockClassName;
    }

    static string GetTXCLLockPickClassName()
    {
        return m_TXCLLockPickClassName;
    }

    static void SetTXCLClassNames(string carlock, string lockpick)
    {
        m_TXCLLockClassName = carlock;
        m_TXCLLockPickClassName = lockpick;
    }

    override void SetActions()
    {
        super.SetActions();
        AddAction(ActionTXCLAttachLock);
        AddAction(ActionTXCLLockpick);
    }
}
#endif
