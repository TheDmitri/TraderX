modded class ItemBase
{
    private static string _txCarLockClassName;
    private static string _txCarLockPickClassName;

    static string GetTXCarLockClassName()
    {
        return _txCarLockClassName;
    }

    static string GetTXCarLockPickClassName()
    {
        return _txCarLockPickClassName;
    }

    static void SetTXCarLockClassNames(string carlock, string lockpick)
    {
        _txCarLockClassName = carlock;
        _txCarLockPickClassName = lockpick;
    }

    override void SetActions()
    {
        super.SetActions();
        AddAction(ActionAttachCarLock);
        AddAction(ActionLockpickVehicle);
    }
}
