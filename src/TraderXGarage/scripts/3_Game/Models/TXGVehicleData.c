// TraderXGarage - Stored vehicle record
// One entry per parked vehicle. The cargo/attachment tree is embedded directly
// (no separate per-cargo files, no map<>) so the whole garage persists as one JSON.

class TXGZoneHealth
{
    string zone;
    float  health01;

    void TXGZoneHealth(string _zone = "", float _health01 = 1.0)
    {
        zone = _zone;
        health01 = _health01;
    }
}

class TXGVehicleData
{
    string vehicleId;       // unique id within the player's garage
    string className;
    float  health;          // global health captured at park-in
    float  fuelFraction;    // 0..1 of fuel tank capacity
    float  oilFraction = 1.0;
    float  coolantFraction = 1.0;
    float  brakeFraction = 1.0;
    bool   fluidsSaved = false;
    int    timestamp;       // YYYYMMDDhhmmss park-in stamp
    bool   hasCarLock = false;
    int    lockCode = -1;   // TraderXCarLock 8-digit code (-1 = no code)
    string lockOwnerUid = "";
    string holdReason = ""; // "" | "INSURANCE" | "IMPOUND"
    int    releaseFee = 0;
    string policyId = "";
    int    noResaleUntil = 0;
    ref array<ref TXGZoneHealth> zones;
    ref array<string> sharedWith;
    ref TXGItemSnapshot snapshot;   // root.childrens = attachments + cargo

    void TXGVehicleData()
    {
        zones = new array<ref TXGZoneHealth>();
        sharedWith = new array<string>();
    }

    void NormalizeAfterLoad()
    {
        if (!zones)
            zones = new array<ref TXGZoneHealth>();
        if (!sharedWith)
            sharedWith = new array<string>();
        if (!fluidsSaved)
        {
            oilFraction = 1.0;
            coolantFraction = 1.0;
            brakeFraction = 1.0;
            fluidsSaved = true;
        }
    }
}
