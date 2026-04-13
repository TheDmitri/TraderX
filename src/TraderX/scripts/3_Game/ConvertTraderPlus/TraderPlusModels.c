// ============================================================================
// TraderPlus V1 JSON models for deserialization
// These mirror the original TraderPlus config file structures exactly
// ============================================================================

// --- TraderPlusGeneralConfig.json ---

class TPV1Currency
{
    string ClassName;
    int Value;
}

class TPV1AcceptedStates
{
    bool AcceptWorn;
    bool AcceptDamaged;
    bool AcceptBadlyDamaged;
    float CoefficientWorn = 0.75;
    float CoefficientDamaged = 0.5;
    float CoefficientBadlyDamaged = 0.25;
}

class TPV1TraderData
{
    int Id;
    string Name;
    string GivenName;
    string Role;
    vector Position;
    vector Orientation;
    ref array<string> Clothes;

    void TPV1TraderData()
    {
        Clothes = new array<string>();
    }
}

class TPV1TraderObject
{
    string ObjectName;
    vector Position;
    vector Orientation;
}

class TPV1GeneralConfig
{
    string Version;
    bool ConvertTraderConfigToTraderPlus;
    bool ConvertTraderConfigToTraderPlusWithStockBasedOnCE;
    bool UseGarageToTradeCar;
    bool DisableHeightFailSafeForReceiptDeployment;
    bool HideInsuranceBtn;
    bool HideGarageBtn;
    bool HideLicenceBtn;
    bool EnableShowAllPrices;
    bool EnableShowAllCheckBox;
    bool IsReceiptTraderOnly;
    bool IsReceiptSaveLock;
    bool IsReceiptSaveAttachment;
    bool IsReceiptSaveCargo;
    float LockPickChance;
    string LicenceKeyWord;
    ref array<string> Licences;
    ref TPV1AcceptedStates AcceptedStates;
    bool StoreOnlyToPristineState;
    ref array<ref TPV1Currency> Currencies;
    ref array<ref TPV1TraderData> Traders;
    ref array<ref TPV1TraderObject> TraderObjects;

    void TPV1GeneralConfig()
    {
        Licences = new array<string>();
        Currencies = new array<ref TPV1Currency>();
        Traders = new array<ref TPV1TraderData>();
        TraderObjects = new array<ref TPV1TraderObject>();
    }
}

// --- TraderPlusPriceConfig.json ---

class TPV1Category
{
    string CategoryName;
    ref array<string> Products;

    void TPV1Category()
    {
        Products = new array<string>();
    }
}

class TPV1PriceConfig
{
    string Version;
    bool EnableAutoCalculation;
    bool EnableAutoDestockAtRestart;
    int EnableDefaultTraderStock;
    ref array<ref TPV1Category> TraderCategories;

    void TPV1PriceConfig()
    {
        TraderCategories = new array<ref TPV1Category>();
    }
}

// --- TraderPlusIDsConfig.json ---

class TPV1TraderIds
{
    int Id;
    ref array<string> Categories;
    ref array<string> LicencesRequired;
    ref array<string> CurrenciesAccepted;

    void TPV1TraderIds()
    {
        Categories = new array<string>();
        LicencesRequired = new array<string>();
        CurrenciesAccepted = new array<string>();
    }
}

class TPV1IDsConfig
{
    string Version;
    ref array<ref TPV1TraderIds> IDs;

    void TPV1IDsConfig()
    {
        IDs = new array<ref TPV1TraderIds>();
    }
}

// --- TraderPlusBankingConfig.json ---

class TPV1BankingConfig
{
    string Version;
    int BankingLogs;
    int IsCreditCarNeededForTransaction;
    float TransactionFees;
    int DefaultStartCurrency;
    int DefaultMaxCurrency;
    ref array<string> CurrenciesAccepted;

    void TPV1BankingConfig()
    {
        CurrenciesAccepted = new array<string>();
    }
}

// --- Account_[STEAMID].json ---

class TPV1BankAccount
{
    string Version;
    string SteamID64;
    string Name;
    int MoneyAmount;
    int MaxAmount;
    ref array<string> Licences;

    void TPV1BankAccount()
    {
        Licences = new array<string>();
    }
}

// --- TraderPlusSafeZoneConfig.json ---

class TPV1SafeZoneLocation
{
    string SafeZoneStatut;
    float X;
    float Y;
    float Radius;
    int Countdown;
}

class TPV1SafeZoneConfig
{
    string Version;
    bool IsHideOutActive;
    bool EnableAfkDisconnect;
    int KickAfterDelay;
    string MsgEnterZone;
    string MsgExitZone;
    string MsgOnLeavingZone;
    string MustRemoveArmband;
    ref array<ref TPV1SafeZoneLocation> SafeAreaLocation;
    int CleanUpTimer;
    ref array<string> WhitelistEntities;
    ref array<string> ObjectsToDelete;
    ref array<string> SZSteamUIDs;
    ref array<string> BlackListedItemInStash;

    void TPV1SafeZoneConfig()
    {
        SafeAreaLocation = new array<ref TPV1SafeZoneLocation>();
        WhitelistEntities = new array<string>();
        ObjectsToDelete = new array<string>();
        SZSteamUIDs = new array<string>();
        BlackListedItemInStash = new array<string>();
    }
}

// --- TraderPlusGarageConfig.json ---

class TPV1GarageNPC
{
    string ClassName;
    vector Position;
    vector Orientation;
    vector ParkingPosition;
    vector ParkingOrientation;
    ref array<string> Clothes;

    void TPV1GarageNPC()
    {
        Clothes = new array<string>();
    }
}

class TPV1GarageConfig
{
    string Version;
    bool UseGarageOnlyToTrade;
    bool VehicleMustHaveLock;
    bool SaveVehicleLock;
    bool SaveVehicleCargo;
    bool SaveVehicleHealth;
    bool SaveVehicleFuel;
    int SavedVehicleInGarageForTradeInHour;
    int MaxVehicleStored;
    int ParkInCost;
    int ParkOutCost;
    bool PayWithBankAccount;
    ref array<string> WhitelistedObjects;
    ref array<ref TPV1GarageNPC> NPCs;

    void TPV1GarageConfig()
    {
        WhitelistedObjects = new array<string>();
        NPCs = new array<ref TPV1GarageNPC>();
    }
}

// --- Garage_[STEAMID].json ---

class TPV1GarageItemCargo
{
    string ItemName;
    int Amount;
    float Health;
    int ExtraData;
}

class TPV1GarageVehicle
{
    string VehicleName;
    int VehicleCargoId;
    int VehicleID;
    int Timestamp;
    float EngineHealth;
    float FuelAmmount;
    int CollateralMoney;
    ref array<ref TPV1GarageItemCargo> ItemsInCargo;

    void TPV1GarageVehicle()
    {
        ItemsInCargo = new array<ref TPV1GarageItemCargo>();
    }
}

class TPV1GarageData
{
    string OwnerName;
    string OwnerID;
    vector ParkingPosition;
    vector ParkingOrientation;
    ref array<ref TPV1GarageVehicle> Vehicles;

    void TPV1GarageData()
    {
        Vehicles = new array<ref TPV1GarageVehicle>();
    }
}
