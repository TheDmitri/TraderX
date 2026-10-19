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
    int    fuelPercent = 100;
    int    oilPercent = 100;
    int    coolantPercent = 100;
    int    brakePercent = 100;
    int    engineHealthLevel = 0;
    int    chassisHealthLevel = 0;
    bool   hasCarLock = false;
    string holdReason = "";
    int    releaseFee = 0;
    int    cargoCount = 0;
    int    repairQuote = 0;
    bool   isInsured = false;
    int    insuranceDaysLeft = 0;
    int    insurancePremiumQuote = 0;
    int    insuranceDeductibleQuote = 0;
    bool   isShared = false;
    int    sharedCount = 0;
    int    insuranceCoveredValue = 0;
    int    insurancePerDay = 0;
    int    insuranceExpiresAt = 0;     // unix seconds, 0 = not insured
    bool   isNearby = false;          // the vehicle next to the player, not stored yet
    string parkInBlockedReason = "";  // why the nearby vehicle can't be parked ("" = can)
    ref array<string> attachmentClasses;
    ref array<string> sharedWith;      // Steam64 ids with access (owner rows only)

    void TXGVehicleRowVM()
    {
        attachmentClasses = new array<string>();
        sharedWith = new array<string>();
    }
}

class TXGVehicleListFile
{
    string ownerUID;
    int    maxSlots;
    ref array<ref TXGVehicleRowVM> vehicles;
    ref array<ref TXGVehicleRowVM> nearby;   // 0 or 1 row: the vehicle next to the player
    ref array<ref TXGSharePlayerVM> people;  // nearby players + people the owner shared with before
    int    bankBalance = -1;                 // -1 = no bank
    int    cashBalance = 0;
    int    nowUnix;

    void TXGVehicleListFile()
    {
        vehicles = new array<ref TXGVehicleRowVM>();
        nearby = new array<ref TXGVehicleRowVM>();
        people = new array<ref TXGSharePlayerVM>();
    }
}

// A player the owner can share a vehicle with (share panel row).
class TXGSharePlayerVM
{
    string uid;
    string name;
    int    distance = -1;   // metres, -1 = not nearby (known from a previous share)
    bool   hasAccess;       // filled client-side for the selected vehicle
    string vehicleId;       // client-side: the vehicle the row acts on
}
