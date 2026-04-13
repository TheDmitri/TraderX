modded class ActionDetach
{
    override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
    {
        if (target && target.GetObject().IsInherited(CarWheel))
        {
            VehicleLockManager manager = VehicleLockManager.Get(target.GetParent());
            if (manager && manager.IsVehicleLocked())
                return false;
        }

        return super.ActionCondition(player, target, item);
    }
}

modded class ActionGetInTransport: ActionBase
{
    override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
    {
        VehicleLockManager manager = VehicleLockManager.Get(target.GetObject());
        if (manager && manager.IsVehicleLocked())
            return false;

        return super.ActionCondition(player, target, item);
    }
};

modded class ActionCarDoors: ActionInteractBase
{
    override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
    {
        VehicleLockManager manager = VehicleLockManager.Get(target.GetParent());
        if (manager && manager.IsVehicleLocked())
            return false;

        return super.ActionCondition(player, target, item);
    }
}

modded class ActionCarDoorsOutside: ActionInteractBase
{
    override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
    {
        VehicleLockManager manager = VehicleLockManager.Get(target.GetParent());
        if (manager && manager.IsVehicleLocked())
            return false;

        return super.ActionCondition(player, target, item);
    }
}

modded class ActionPushObject : ActionContinuousBase
{
    override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
    {
        VehicleLockManager manager = VehicleLockManager.Get(target.GetObject());
        if (manager && manager.IsVehicleLocked())
            return false;

        return super.ActionCondition(player, target, item);
    }
}
