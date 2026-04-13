class VehicleTypeHelper
{
    static EVehicleLockType GetVehicleType(IEntity entity)
    {
        if (!entity)
            return EVehicleLockType.UNKNOWN;

        Object obj = Object.Cast(entity);
        string typeName = "";
        if (obj)
        {
            typeName = obj.GetType();
            typeName.ToLower();
        }

        if (entity.IsInherited(BoatScript))
            return EVehicleLockType.BOAT;

        if (entity.IsInherited(CarScript))
        {
            if (typeName.Contains("heli") || typeName.Contains("helicopter") || typeName.Contains("rffsheli"))
                return EVehicleLockType.HELI;
            return EVehicleLockType.CAR;
        }

        return EVehicleLockType.UNKNOWN;
    }

    static string GetVehicleTypeName(EVehicleLockType vehicleType)
    {
        switch (vehicleType)
        {
            case EVehicleLockType.CAR:
                return "Car";
            case EVehicleLockType.BOAT:
                return "Boat";
            case EVehicleLockType.HELI:
                return "Helicopter";
            default:
                return "Unknown";
        }

        return "Unknown";
    }

    static TXVehicleLockpickConfig GetLockpickConfigForVehicle(IEntity entity)
    {
        TXCarLockConfigService configService = GetTXCarLockConfigService();
        if (!configService)
            return null;

        TXCarLockSettings settings = configService.GetSettings();
        if (!settings || !settings.lockpickingSettings)
            return null;

        EVehicleLockType vehicleType = GetVehicleType(entity);
        return settings.lockpickingSettings.GetConfigForVehicleType(vehicleType);
    }
}
