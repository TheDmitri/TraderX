// TraderXGarage - Stored vehicle record
// One entry per parked vehicle. The cargo/attachment tree is embedded directly
// (no separate per-cargo files, no map<>) so the whole garage persists as one JSON.

class TXGVehicleData
{
    string vehicleId;       // unique id within the player's garage
    string className;
    float  health;          // global health captured at park-in
    float  fuelFraction;    // 0..1 of fuel tank capacity
    int    timestamp;       // YYYYMMDDhhmmss park-in stamp
    int    lockCode;        // reserved for future TraderXCarLock integration (US-032)
    ref TXGItemSnapshot snapshot;   // root.childrens = attachments + cargo

    void TXGVehicleData() {}
}
