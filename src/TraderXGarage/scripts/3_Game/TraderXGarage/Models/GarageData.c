// TraderXGarage - GarageData
// Per-player garage database. Stores parked vehicles. All fields camelCase.

class GarageData
{
	string ownerName;
	string ownerId;
	vector parkingPosition;
	vector parkingOrientation;
	ref array<ref VehicleData> vehicles;

	void GarageData()
	{
		vehicles = new array<ref VehicleData>();
	}

	void DefaultGarageData(string name, string id, string steamId)
	{
		ownerName = name;
		ownerId = id;
		parkingPosition = "0 0 0";
		parkingOrientation = "0 0 0";
		vehicles = new array<ref VehicleData>();
	}

	void AddVehicle(string name, int cargoId, int vehId, int ts, float engine, float fuel, ref array<ref ItemCargo> cargo, int collateral)
	{
		ref VehicleData vData = new VehicleData(name, cargoId, vehId, ts, engine, fuel, cargo, collateral);
		vehicles.Insert(vData);
	}

	int GetVehicleCount()
	{
		return vehicles.Count();
	}

	void RemoveVehicle(int index)
	{
		if (index >= 0 && index < vehicles.Count())
			vehicles.Remove(index);
	}

	void Save(string id)
	{
		if (!FileExist(TXG_DB_DIR_SERVER))
			MakeDirectory(TXG_DB_DIR_SERVER);

		string path = TXG_DATABASE_PREFIX + id + ".json";
		JsonFileLoader<GarageData>.JsonSaveFile(path, this);
	}

	void DeleteFile(string id)
	{
		string path = TXG_DATABASE_PREFIX + id + ".json";
		if (FileExist(path))
			DeleteFile(path);
	}

	static ref GarageData Load(string id, string playerName)
	{
		ref GarageData data;

		if (!FileExist(TXG_DB_DIR_SERVER))
			MakeDirectory(TXG_DB_DIR_SERVER);

		string path = TXG_DATABASE_PREFIX + id + ".json";

		if (FileExist(path))
		{
			data = new GarageData();
			JsonFileLoader<GarageData>.JsonLoadFile(path, data);
		}
		else
		{
			data = new GarageData();
			data.DefaultGarageData(playerName, id, id);
			data.Save(id);
		}

		return data;
	}
}
