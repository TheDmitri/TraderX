// TraderXGarage - GarageItemData
// Full recursive inventory snapshot for vehicle cargo persistence. All fields camelCase.

class GarageItemData
{
	string item;
	int quantity;
	float health;
	int magAmmoCount;
	int liquidType;
	int foodStage;
	int extraData;
	float temperature;
	float wetness;
	float energy;
	string locationType;
	int locationIndex;
	int locationRow;
	int locationColumn;
	bool locationFlipped;
	ref array<ref GarageItemData> children;

	void GarageItemData()
	{
		children = new array<ref GarageItemData>();
	}

	static ref GarageItemData Load(int playerId, int cargoId)
	{
		string dirPath = TXG_DB_DIR_SERVER + playerId.ToString() + "\\";
		string filePath = dirPath + cargoId.ToString() + ".json";

		if (!FileExist(filePath))
			return null;

		ref GarageItemData data = new GarageItemData();
		JsonFileLoader<GarageItemData>.JsonLoadFile(filePath, data);
		return data;
	}

	void Save(int playerId, int cargoId)
	{
		string dirPath = TXG_DB_DIR_SERVER + playerId.ToString() + "\\";

		if (!FileExist(dirPath))
			MakeDirectory(dirPath);

		string filePath = dirPath + cargoId.ToString() + ".json";
		JsonFileLoader<GarageItemData>.JsonSaveFile(filePath, this);
	}

	static void DeleteCargo(int playerId, int cargoId)
	{
		string dirPath = TXG_DB_DIR_SERVER + playerId.ToString() + "\\";
		string filePath = dirPath + cargoId.ToString() + ".json";

		if (FileExist(filePath))
			DeleteFile(filePath);
	}
}
