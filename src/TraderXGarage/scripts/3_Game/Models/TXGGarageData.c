// TraderXGarage - Per-player garage persistence
// File: $profile:TraderX/TraderXGarage/Database/Garage_<uid>.json
// Vehicles persist as array<ref TXGVehicleData> (no map<> — see memory dayz-jsonfileloader-no-map).

class TXGGarageData
{
    string version = TXGConstants.DATA_VERSION;
    string ownerUID;
    string ownerName;
    int    nextVehicleId;   // monotonic counter -> stable unique vehicle ids
    ref array<ref TXGVehicleData> vehicles;

    void TXGGarageData()
    {
        vehicles = new array<ref TXGVehicleData>();
    }

    static string GetFilePath(string uid)
    {
        return TXGConstants.DB_DIR + "Garage_" + uid + ".json";
    }

    string AllocateVehicleId()
    {
        nextVehicleId = nextVehicleId + 1;
        return "V" + nextVehicleId.ToString();
    }

    TXGVehicleData FindVehicle(string vehicleId)
    {
        foreach (TXGVehicleData v : vehicles)
        {
            if (v && v.vehicleId == vehicleId)
                return v;
        }
        return null;
    }

    void RemoveVehicle(string vehicleId)
    {
        for (int i = 0; i < vehicles.Count(); i++)
        {
            if (vehicles[i] && vehicles[i].vehicleId == vehicleId)
            {
                vehicles.RemoveOrdered(i);
                return;
            }
        }
    }

    void Save()
    {
        if (!FileExist(TXGConstants.DB_DIR))
            MakeDirectory(TXGConstants.DB_DIR);

        JsonFileLoader<TXGGarageData>.JsonSaveFile(GetFilePath(ownerUID), this);
    }

    static ref TXGGarageData Load(string uid, string name)
    {
        if (!FileExist(TXGConstants.DB_DIR))
            MakeDirectory(TXGConstants.DB_DIR);

        string path = GetFilePath(uid);
        ref TXGGarageData data;

        if (FileExist(path))
        {
            data = new TXGGarageData();
            JsonFileLoader<TXGGarageData>.JsonLoadFile(path, data);

            if (!data.vehicles)
                data.vehicles = new array<ref TXGVehicleData>();

            if (data.version != TXGConstants.DATA_VERSION)
            {
                data.version = TXGConstants.DATA_VERSION;
                data.Save();
            }
        }
        else
        {
            data = new TXGGarageData();
            data.ownerUID = uid;
            data.ownerName = name;
            data.Save();
            GetTXGLog().LogInfo("[TXG] New garage created for " + name + " (" + uid + ")");
        }

        return data;
    }
}
