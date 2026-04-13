// TraderXGarage - VehicleData
// Individual stored vehicle record. All fields camelCase.

class VehicleData
{
	string vehicleName;
	int vehicleCargoId;
	int vehicleId;
	int timestamp;
	float engineHealth;
	float fuelAmount;
	int collateralMoney;
	ref array<ref ItemCargo> itemsInCargo;

	// Lock data (serialized from CarLock provider): [hasLock, isLocked, ownerUID, password]
	ref array<int> lockData;

	void VehicleData(string name = "", int cargoId = 0, int vehId = 0, int ts = 0, float engine = 0, float fuel = 0, ref array<ref ItemCargo> cargo = null, int collateral = 0)
	{
		vehicleName = name;
		vehicleCargoId = cargoId;
		vehicleId = vehId;
		timestamp = ts;
		engineHealth = engine;
		fuelAmount = fuel;
		collateralMoney = collateral;

		if (cargo)
			itemsInCargo = cargo;
		else
			itemsInCargo = new array<ref ItemCargo>();
	}

	void DeleteCargoIfExists(int playerId)
	{
		if (vehicleCargoId != 0)
			GarageItemData.DeleteCargo(playerId, vehicleCargoId);
	}
}
