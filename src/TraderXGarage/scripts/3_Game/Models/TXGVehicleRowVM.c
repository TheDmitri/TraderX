// TraderXGarage - Client view-models
// TXGVehicleRowVM   : one stored vehicle, bound by the ForEach list (array<ref ...>).
// TXGVehicleListFile: JSON envelope used to ship the list server -> client over RPC.

class TXGVehicleRowVM
{
    string vehicleId;
    string className;
    string displayName;
    int    healthLevel;     // 0..4 (PRISTINE..RUINED)
    string healthText;

    void TXGVehicleRowVM() {}
}

class TXGVehicleListFile
{
    string ownerUID;
    int    maxSlots;
    ref array<ref TXGVehicleRowVM> vehicles;

    void TXGVehicleListFile()
    {
        vehicles = new array<ref TXGVehicleRowVM>();
    }
}
