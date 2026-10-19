#ifndef CarLock
// TraderPlus V1 defines "CarLock" and owns the CarLock item: TraderX CarLock stands down when it is loaded (see TXCLTraderPlusNotice).
// Block normal interactions (enter, doors, wheel detach, push) while a vehicle is TXCL-locked.

modded class ActionGetInTransport
{
    override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
    {
        TXCLVehicleLockManager manager = TXCLVehicleLockManager.Get(target.GetObject());
        if (manager && manager.IsVehicleLocked())
            return false;

        return super.ActionCondition(player, target, item);
    }
}

modded class ActionCarDoors
{
    override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
    {
        TXCLVehicleLockManager manager = TXCLVehicleLockManager.Get(target.GetParent());
        if (manager && manager.IsVehicleLocked())
            return false;

        return super.ActionCondition(player, target, item);
    }
}

modded class ActionCarDoorsOutside
{
    override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
    {
        TXCLVehicleLockManager manager = TXCLVehicleLockManager.Get(target.GetParent());
        if (manager && manager.IsVehicleLocked())
            return false;

        return super.ActionCondition(player, target, item);
    }
}

modded class ActionDetach
{
    override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
    {
        if (target && target.GetObject() && target.GetObject().IsInherited(CarWheel))
        {
            TXCLVehicleLockManager manager = TXCLVehicleLockManager.Get(target.GetParent());
            if (manager && manager.IsVehicleLocked())
                return false;
        }

        return super.ActionCondition(player, target, item);
    }
}

modded class ActionPushObject
{
    override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
    {
        TXCLVehicleLockManager manager = TXCLVehicleLockManager.Get(target.GetObject());
        if (manager && manager.IsVehicleLocked())
            return false;

        return super.ActionCondition(player, target, item);
    }
}
#endif
