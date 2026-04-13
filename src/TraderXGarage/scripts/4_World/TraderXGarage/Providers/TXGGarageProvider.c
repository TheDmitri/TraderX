/**
 * TraderXGarage implementation of ITraderXGarageProvider.
 * Bridges the TraderX provider interface to the existing GarageService.
 *
 * Registered automatically when TraderXGarage module loads.
 */
class TXGGarageProvider : ITraderXGarageProvider
{
    private ref ScriptInvoker m_OnVehicleListReceived;
    private ref ScriptInvoker m_OnParkOutResult;

    void TXGGarageProvider()
    {
        m_OnVehicleListReceived = new ScriptInvoker();
        m_OnParkOutResult = new ScriptInvoker();
    }

    override int GetStoredVehicleCount(string playerUID)
    {
        if (!GetGame().IsServer())
            return 0;

        ref GarageData garageData = GarageData.Load(playerUID, "");
        if (!garageData || !garageData.vehicles)
            return 0;

        return garageData.vehicles.Count();
    }

    override int GetParkInCost()
    {
        GarageService service = GetGarageService();
        if (!service || !service.GetSettings())
            return 0;

        return service.GetSettings().parkInCost;
    }

    override int GetParkOutCost()
    {
        GarageService service = GetGarageService();
        if (!service || !service.GetSettings())
            return 0;

        return service.GetSettings().parkOutCost;
    }

    override bool IsAvailableAtNpc(int npcId)
    {
        // Garage NPCs use the special TXG_GARAGE_NPC_ID (-3)
        // but could also be configured per-NPC in the future
        return npcId == TXG_GARAGE_NPC_ID;
    }

    override void RequestVehicleList(int playerLowId, vector npcPosition)
    {
        // Client sends RPC to server requesting vehicle list
        Param3<int, vector, bool> params = new Param3<int, vector, bool>(playerLowId, npcPosition, false);
        GetDayZularRPCManager().SendRPC(TXG_RPC_NAMESPACE, TXG_RPC_GARAGE_REQUEST, params, true, null);
    }

    override ScriptInvoker GetOnVehicleListReceived()
    {
        return m_OnVehicleListReceived;
    }

    override void RequestParkOut(int playerLowId, int vehicleIndex, string vehicleName, vector npcPosition)
    {
        Param4<int, int, string, vector> params = new Param4<int, int, string, vector>(playerLowId, vehicleIndex, vehicleName, npcPosition);
        GetDayZularRPCManager().SendRPC(TXG_RPC_NAMESPACE, TXG_RPC_PARK_OUT, params, true, null);
    }

    override ScriptInvoker GetOnParkOutResult()
    {
        return m_OnParkOutResult;
    }

    override bool UseBankPayment()
    {
        GarageService service = GetGarageService();
        if (!service || !service.GetSettings())
            return false;

        return service.GetSettings().payWithBankAccount;
    }

    // Called by GarageRpcService when vehicle list response arrives on client
    void OnVehicleListResponseReceived(TStringArray vehicleNames, TIntArray healthValues, int responseCode)
    {
        m_OnVehicleListReceived.Invoke(vehicleNames, healthValues, responseCode);
    }

    // Called by GarageRpcService when park-out result arrives on client
    void OnParkOutResultReceived(bool success, string message)
    {
        m_OnParkOutResult.Invoke(success, message);
    }
}

// Global accessor
static ref GarageService g_GarageService;

GarageService GetGarageService()
{
    if (!g_GarageService)
        g_GarageService = GarageService.Cast(DZlrServiceRegistry.GetInstance().GetService(GarageService));
    return g_GarageService;
}
