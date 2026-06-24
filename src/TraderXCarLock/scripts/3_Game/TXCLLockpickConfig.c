// Per-vehicle-type lockpick tuning. Mirrors TheLab's lockpicking config, TXCL-prefixed.

class TXCLVehicleLockpickConfig
{
    string lockpickClassName;
    float  lockpickTime;
    float  lockpickChance;
    float  lockpickDamagePerAttempt;
    bool   consumeLockpickOnSuccess;
    bool   consumeLockpickOnFailure;

    void TXCLVehicleLockpickConfig()
    {
        lockpickClassName = "CarLockPick";
        lockpickTime = 60.0;
        lockpickChance = 0.33;
        lockpickDamagePerAttempt = 0.34;
        consumeLockpickOnSuccess = false;
        consumeLockpickOnFailure = false;
    }

    void SetDefaults(ETXCLVehicleType vehicleType)
    {
        switch (vehicleType)
        {
            case ETXCLVehicleType.CAR:
                lockpickClassName = "CarLockPick";
                lockpickTime = 60.0;
                lockpickChance = 0.33;
                lockpickDamagePerAttempt = 0.34;
                consumeLockpickOnSuccess = false;
                consumeLockpickOnFailure = false;
                break;

            case ETXCLVehicleType.BOAT:
                lockpickClassName = "BoatLockPick";
                lockpickTime = 45.0;
                lockpickChance = 0.40;
                lockpickDamagePerAttempt = 0.25;
                consumeLockpickOnSuccess = false;
                consumeLockpickOnFailure = false;
                break;

            case ETXCLVehicleType.HELI:
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

class TXCLLockpickingSettings
{
    ref TXCLVehicleLockpickConfig carConfig;
    ref TXCLVehicleLockpickConfig boatConfig;
    ref TXCLVehicleLockpickConfig heliConfig;

    void TXCLLockpickingSettings()
    {
        carConfig = new TXCLVehicleLockpickConfig();
        carConfig.SetDefaults(ETXCLVehicleType.CAR);

        boatConfig = new TXCLVehicleLockpickConfig();
        boatConfig.SetDefaults(ETXCLVehicleType.BOAT);

        heliConfig = new TXCLVehicleLockpickConfig();
        heliConfig.SetDefaults(ETXCLVehicleType.HELI);
    }

    TXCLVehicleLockpickConfig GetConfigForVehicleType(ETXCLVehicleType vehicleType)
    {
        switch (vehicleType)
        {
            case ETXCLVehicleType.CAR:
                return carConfig;
            case ETXCLVehicleType.BOAT:
                return boatConfig;
            case ETXCLVehicleType.HELI:
                return heliConfig;
            default:
                return carConfig;
        }

        return carConfig;
    }
}
