// TraderXGarage - RPC data models
// Flat, serializable DTOs marshaled over RPC as Param1<ref DTO> (never packed strings / JSON).
// Primitives + string + ref array<...> only — see CLAUDE.md DayZular RPC rules.

// Server -> client: lightweight config the client needs (costs, slots, npc class names).
class TXGConfigDTO
{
    int parkInCost;
    int parkOutCost;
    int maxSlots;
    ref array<string> npcClasses;

    void TXGConfigDTO()
    {
        npcClasses = new array<string>();
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
