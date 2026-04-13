// TraderXGarage - GarageSettings
// Server configuration model. Serialized to JSON. All fields use camelCase.

class GarageSettings
{
	string version = "1.0";

	bool useGarageOnlyToTrade = false;
	bool vehicleMustHaveLock = true;
	bool saveVehicleLock = true;
	bool saveVehicleCargo = false;
	bool saveVehicleHealth = true;
	bool saveVehicleFuel = true;

	int savedVehicleInGarageForTradeInHours = 1;
	int maxVehiclesStored = 5;
	int parkInCost = 100;
	int parkOutCost = 100;
	bool payWithBankAccount = false;

	ref TStringArray whitelistedObjects = new TStringArray();
	ref array<ref GarageNpc> npcs = new array<ref GarageNpc>();

	// Notification messages
	string msgParkingNotAvailable = "The parking area is not available!";
	string msgNotEnoughMoney = "You don't have enough money!";
	string msgNotRightToPark = "You don't have the right to park this vehicle!";
	string msgCarHasMember = "The vehicle still has passengers inside!";
	string msgParkInFail = "The vehicle could not be parked correctly.";
	string msgParkInSuccess = "The vehicle was successfully parked.";
	string msgParkOutFail = "The vehicle could not be spawned!";
	string msgParkOutSuccess = "The vehicle was successfully retrieved!";
	string msgMaxVehiclesReached = "Maximum number of stored vehicles reached!";
	string msgTradeVehicleWarning = "The vehicle has been stored for %1 hour(s). After this point it will be lost — trade it or retrieve it quickly.";
	string msgTradeVehicleDeleted = "The stored vehicle exceeded its inactivity limit and has been deleted.";

	void GarageSettings()
	{
		whitelistedObjects = new TStringArray();
		npcs = new array<ref GarageNpc>();
	}

	void DefaultSettings()
	{
		ref GarageNpc defaultNpc = new GarageNpc(
			"TraderPlusSign",
			Vector(3723.84, 402.012, 5990.04),
			Vector(0, 0, 0),
			Vector(3719.71, 402.012, 5994.85),
			Vector(-42, 0, 0)
		);
		npcs.Insert(defaultNpc);
	}

	void Save()
	{
		if (!FileExist(TXG_CONFIG_DIR_SERVER))
			MakeDirectory(TXG_CONFIG_DIR_SERVER);

		JsonFileLoader<GarageSettings>.JsonSaveFile(TXG_CONFIG_FILE, this);
	}

	void CheckVersion()
	{
		if (version != "1.0")
		{
			version = "1.0";
			Save();
		}
	}

	static ref GarageSettings Load()
	{
		ref GarageSettings settings;

		if (!FileExist(TXG_CONFIG_DIR_SERVER))
			MakeDirectory(TXG_CONFIG_DIR_SERVER);

		if (FileExist(TXG_CONFIG_FILE))
		{
			settings = new GarageSettings();
			JsonFileLoader<GarageSettings>.JsonLoadFile(TXG_CONFIG_FILE, settings);
			settings.CheckVersion();
		}
		else
		{
			settings = new GarageSettings();
			settings.DefaultSettings();
			settings.Save();
		}

		return settings;
	}
}
