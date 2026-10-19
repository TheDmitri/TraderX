#ifndef CarLock
// TraderPlus V1 defines "CarLock" and owns the CarLock item: TraderX CarLock stands down when it is loaded (see TXCLTraderPlusNotice).
// Classifies a vehicle into a lock type and resolves its lockpick config from module settings.

class TXCLVehicleTypeHelper
{
    static ETXCLVehicleType GetVehicleType(IEntity entity)
    {
        if (!entity)
            return ETXCLVehicleType.UNKNOWN;

        Object obj = Object.Cast(entity);
        string typeName = "";
        if (obj)
        {
            typeName = obj.GetType();
            typeName.ToLower();
        }

        if (entity.IsInherited(BoatScript))
            return ETXCLVehicleType.BOAT;

        if (entity.IsInherited(CarScript))
        {
            if (typeName.Contains("heli") || typeName.Contains("helicopter"))
                return ETXCLVehicleType.HELI;
            return ETXCLVehicleType.CAR;
        }

        return ETXCLVehicleType.UNKNOWN;
    }

    static string GetVehicleTypeName(ETXCLVehicleType vehicleType)
    {
        switch (vehicleType)
        {
            case ETXCLVehicleType.CAR:
                return "Car";
            case ETXCLVehicleType.BOAT:
                return "Boat";
            case ETXCLVehicleType.HELI:
                return "Helicopter";
            default:
                return "Unknown";
        }

        return "Unknown";
    }

    static TXCLVehicleLockpickConfig GetLockpickConfigForVehicle(IEntity entity)
    {
        TXCLModule module = GetTXCLModule();
        if (!module || !module.GetSettings() || !module.GetSettings().lockpickingSettings)
            return null;

        ETXCLVehicleType vehicleType = GetVehicleType(entity);
        return module.GetSettings().lockpickingSettings.GetConfigForVehicleType(vehicleType);
    }
}
#endif
