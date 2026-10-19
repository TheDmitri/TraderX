// TraderXGarage - RPC data models
// Flat, serializable DTOs marshaled over RPC as Param1<ref DTO> (never packed strings / JSON).
// Primitives + string + ref array<...> only — see CLAUDE.md DayZular RPC rules.

class TXGNpcSpotDTO
{
    int    npcId;
    vector npcPos;
    vector spotPos;
    vector spotOri;
    vector spotSize;

    void TXGNpcSpotDTO(int _npcId = -1, vector _npcPos = vector.Zero, vector _spotPos = vector.Zero, vector _spotOri = vector.Zero, vector _spotSize = vector.Zero)
    {
        npcId = _npcId;
        npcPos = _npcPos;
        spotPos = _spotPos;
        spotOri = _spotOri;
        spotSize = _spotSize;
    }
}

// Server -> client: lightweight config the client needs (costs, slots, npc class names + ids + spots).
class TXGConfigDTO
{
    int  parkInCost;
    int  parkOutCost;
    int  maxSlots;
    bool enableRepairs;
    bool enableInsurance;
    int  insuranceDefaultDays;
    int  insuranceMaxDays;
    bool isAdmin;
    ref array<string> npcClasses;
    ref array<int> npcIds;
    ref array<ref TXGNpcSpotDTO> npcSpots;

    void TXGConfigDTO()
    {
        npcClasses = new array<string>();
        npcIds = new array<int>();
        npcSpots = new array<ref TXGNpcSpotDTO>();
    }
}

// Client -> server: retrieve a stored vehicle.
class TXGParkOutRequest
{
    string vehicleId;
    int    npcId;

    void TXGParkOutRequest(string _vehicleId = "", int _npcId = -1)
    {
        vehicleId = _vehicleId;
        npcId = _npcId;
    }
}

// Client -> server: repair, insure, or share a stored vehicle.
class TXGVehicleActionRequest
{
    string vehicleId;
    int    intParam;
    string strParam;

    void TXGVehicleActionRequest(string _vehicleId = "", int _intParam = 0, string _strParam = "")
    {
        vehicleId = _vehicleId;
        intParam = _intParam;
        strParam = _strParam;
    }
}

// Server -> client: one-shot confirmation/error toast.
class TXGNotifyDTO
{
    bool   success;
    string message;

    void TXGNotifyDTO(bool _success = false, string _message = "")
    {
        success = _success;
        message = _message;
    }
}
