// A vehicle packed into a receipt: saved server side at TraderXGarage/Receipts/<receiptId>.json while
// the receipt exists. The item only carries the id, so a duplicated item cannot bring the vehicle twice.
class TXGReceiptData
{
    string version = "1";
    string receiptId;
    string packerUid;
    string packerName;
    int    packedAt;
    ref TXGVehicleData vehicle;

    static string GetFilePath(string id)
    {
        return TXGConstants.RECEIPT_DIR + id + ".json";
    }

    static TXGReceiptData Load(string id)
    {
        if (id == "" || !FileExist(GetFilePath(id)))
            return null;

        TXGReceiptData data = new TXGReceiptData();
        string error;
        if (!JsonFileLoader<TXGReceiptData>.LoadFile(GetFilePath(id), data, error) || !data.vehicle)
        {
            GetTXGLog().LogWarning("[TXG] Unreadable receipt " + id + " " + error);
            return null;
        }
        data.vehicle.NormalizeAfterLoad();
        return data;
    }

    bool Save()
    {
        if (!FileExist(TXGConstants.ROOT_DIR))
            MakeDirectory(TXGConstants.ROOT_DIR);
        if (!FileExist(TXGConstants.CONFIG_DIR))
            MakeDirectory(TXGConstants.CONFIG_DIR);
        if (!FileExist(TXGConstants.RECEIPT_DIR))
            MakeDirectory(TXGConstants.RECEIPT_DIR);

        string error;
        if (!JsonFileLoader<TXGReceiptData>.SaveFile(GetFilePath(receiptId), this, error))
        {
            GetTXGLog().LogError("[TXG] Could not save receipt " + receiptId + " " + error);
            return false;
        }
        return FileExist(GetFilePath(receiptId));
    }

    static void Delete(string id)
    {
        if (id != "" && FileExist(GetFilePath(id)))
            DeleteFile(GetFilePath(id));
    }
}

// TraderPlus V1 vehicle part lists, kept by the TraderX import in TraderXVehiclePartsConfig.json.
// Old TraderPlus receipts ("Receipt") point to an entry by index; Height lifts the vehicle on deploy.
class TXGTraderPlusVehicleParts
{
    string VehicleName;
    float  Height;
    ref array<string> VehicleParts;
}

class TXGTraderPlusVehiclePartsConfig
{
    ref array<ref TXGTraderPlusVehicleParts> VehiclesParts;

    static TXGTraderPlusVehiclePartsConfig Load()
    {
        TXGTraderPlusVehiclePartsConfig config = new TXGTraderPlusVehiclePartsConfig();
        if (FileExist(TXGConstants.TRADERPLUS_VEHICLE_PARTS_FILE))
        {
            string error;
            JsonFileLoader<TXGTraderPlusVehiclePartsConfig>.LoadFile(TXGConstants.TRADERPLUS_VEHICLE_PARTS_FILE, config, error);
        }
        if (!config.VehiclesParts)
            config.VehiclesParts = new array<ref TXGTraderPlusVehicleParts>();
        return config;
    }

    TXGTraderPlusVehicleParts FindByClass(string className)
    {
        foreach (TXGTraderPlusVehicleParts entry : VehiclesParts)
        {
            if (entry && entry.VehicleName == className)
                return entry;
        }
        return null;
    }
}
