class TXVehicleLockpickConfig
{
    string lockpickClassName;
    float lockpickTime;
    float lockpickChance;
    float lockpickDamagePerAttempt;
    bool consumeLockpickOnSuccess;
    bool consumeLockpickOnFailure;

    void TXVehicleLockpickConfig()
    {
        lockpickClassName = "CarLockPick";
        lockpickTime = 60.0;
        lockpickChance = 0.33;
        lockpickDamagePerAttempt = 0.34;
        consumeLockpickOnSuccess = false;
        consumeLockpickOnFailure = false;
    }

    void SetDefaults(EVehicleLockType vehicleType)
    {
        switch (vehicleType)
        {
            case EVehicleLockType.CAR:
                lockpickClassName = "CarLockPick";
                lockpickTime = 60.0;
                lockpickChance = 0.33;
                lockpickDamagePerAttempt = 0.34;
                consumeLockpickOnSuccess = false;
                consumeLockpickOnFailure = false;
                break;

            case EVehicleLockType.BOAT:
                lockpickClassName = "BoatLockPick";
                lockpickTime = 45.0;
                lockpickChance = 0.40;
                lockpickDamagePerAttempt = 0.25;
                consumeLockpickOnSuccess = false;
                consumeLockpickOnFailure = false;
                break;

            case EVehicleLockType.HELI:
                lockpickClassName = "HeliLockPick";
                lockpickTime = 120.0;
                lockpickChance = 0.15;
                lockpickDamagePerAttempt = 0.50;
                consumeLockpickOnSuccess = false;
                consumeLockpickOnFailure = false;
                break;

            default:
                lockpickClassName = "CarLockPick";
                lockpickTime = 60.0;
                lockpickChance = 0.33;
                lockpickDamagePerAttempt = 0.34;
                consumeLockpickOnSuccess = false;
                consumeLockpickOnFailure = false;
                break;
        }
    }
}

class TXLockpickingSettings
{
    ref TXVehicleLockpickConfig carConfig;
    ref TXVehicleLockpickConfig boatConfig;
    ref TXVehicleLockpickConfig heliConfig;

    void TXLockpickingSettings()
    {
        carConfig = new TXVehicleLockpickConfig();
        carConfig.SetDefaults(EVehicleLockType.CAR);

        boatConfig = new TXVehicleLockpickConfig();
        boatConfig.SetDefaults(EVehicleLockType.BOAT);

        heliConfig = new TXVehicleLockpickConfig();
        heliConfig.SetDefaults(EVehicleLockType.HELI);
    }

    TXVehicleLockpickConfig GetConfigForVehicleType(EVehicleLockType vehicleType)
    {
        switch (vehicleType)
        {
            case EVehicleLockType.CAR:
                return carConfig;
            case EVehicleLockType.BOAT:
                return boatConfig;
            case EVehicleLockType.HELI:
                return heliConfig;
            default:
                return carConfig;
        }

        return carConfig;
    }
}
