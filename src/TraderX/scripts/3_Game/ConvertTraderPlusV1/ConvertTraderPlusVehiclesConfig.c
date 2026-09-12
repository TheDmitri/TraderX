// TraderPlus V1 vehicle part-lists (VehiclesConfig) have NO runtime consumer in TraderX yet —
// the receipt pack/deploy paradigm was replaced by the Garage parking marketplace, and the
// insurance feature that would use a parts list is not built (see docs/TraderPlus_Migration_Gaps.md).
//
// Rather than silently drop an admin's tuned data on migration, we parse + re-serialize it to a
// clean TraderX-named file so a future feature can pick it up. This converter wires NOTHING.
class oldTraderPlusVehicleEntry
{
    string            VehicleName;
    float             Height;
    ref array<string> VehicleParts;

    void oldTraderPlusVehicleEntry()
    {
        VehicleParts = new array<string>();
    }
}

class oldTraderPlusVehiclesConfig
{
    string Version = "";
    ref array<ref oldTraderPlusVehicleEntry> VehiclesParts;

    void oldTraderPlusVehiclesConfig()
    {
        VehiclesParts = new array<ref oldTraderPlusVehicleEntry>();
    }

    static void ConvertOldConfigToNew()
    {
        string path = TraderXLegacyConfigPath.Resolve("TraderXVehiclesConfig.json", "TraderPlusVehiclesConfig.json");
        if (path == string.Empty)
            return;

        oldTraderPlusVehiclesConfig config = new oldTraderPlusVehiclesConfig();
        GetTraderXLogger().LogInfo("[TraderX Migration] Loading legacy vehicles config: " + path);
        TraderXJsonLoader<oldTraderPlusVehiclesConfig>.LoadFromFile(path, config);

        TraderXJsonLoader<oldTraderPlusVehiclesConfig>.SaveToFile(TRADERX_VEHICLE_PARTS_FILE, config);
        GetTraderXLogger().LogInfo("[TraderX Migration] Preserved " + config.VehiclesParts.Count() + " vehicle part-list(s) to " + TRADERX_VEHICLE_PARTS_FILE + " — NOTE: no runtime consumer yet (receipt/insurance feature pending). Kept for forward-compat.");

        TraderXLegacyConfigPath.Archive(path, "TraderPlusVehiclesConfig.json");
    }
}
