modded class ActionConstructor
{
    override void RegisterActions(TTypenameArray actions)
    {
        super.RegisterActions(actions);
        actions.Insert(ActionAttachCarLock);
        actions.Insert(ActionDetachCarLock);
        actions.Insert(ActionLockpickVehicle);

        actions.Insert(ActionCarLock);
        actions.Insert(ActionCarUnlock);
        actions.Insert(ActionCarLockMenu);
    }
}
