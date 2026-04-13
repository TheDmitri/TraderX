/**
 * Abstract garage provider interface for TraderX.
 * External mods (e.g., TraderXGarage) extend this class and register
 * via TraderXProviderRegistry to enable garage access from the trader UI.
 *
 * If no provider is registered, the garage tab is hidden in the trader menu.
 */
class ITraderXGarageProvider
{
    /**
     * Get the number of vehicles stored for a player.
     * Client-side: uses cached data. Server-side: reads from storage.
     */
    int GetStoredVehicleCount(string playerUID)
    {
        return 0;
    }

    /**
     * Get park-in cost from garage settings.
     */
    int GetParkInCost()
    {
        return 0;
    }

    /**
     * Get park-out cost from garage settings.
     */
    int GetParkOutCost()
    {
        return 0;
    }

    /**
     * Whether garage functionality is available at a given NPC.
     * @param npcId The trader NPC ID to check
     */
    bool IsAvailableAtNpc(int npcId)
    {
        return false;
    }

    /**
     * Request the vehicle list from the server for display in the trader UI.
     * Results arrive asynchronously via GetOnVehicleListReceived().
     * @param playerLowId The player's low network ID
     * @param npcPosition The NPC position for proximity validation
     */
    void RequestVehicleList(int playerLowId, vector npcPosition)
    {
    }

    /**
     * Event invoker that fires when vehicle list data is received from the server.
     * Subscribers receive: (TStringArray vehicleNames, TIntArray healthValues, int responseCode)
     */
    ScriptInvoker GetOnVehicleListReceived()
    {
        return null;
    }

    /**
     * Request a park-out operation from the garage.
     * @param playerLowId The player's low network ID
     * @param vehicleIndex Index of the vehicle in the stored list
     * @param vehicleName Class name of the vehicle
     * @param npcPosition NPC position for spawn location
     */
    void RequestParkOut(int playerLowId, int vehicleIndex, string vehicleName, vector npcPosition)
    {
    }

    /**
     * Event invoker that fires when a park-out result is received.
     * Subscribers receive: (bool success, string message)
     */
    ScriptInvoker GetOnParkOutResult()
    {
        return null;
    }

    /**
     * Whether the garage uses bank account for payment (vs inventory currency).
     */
    bool UseBankPayment()
    {
        return false;
    }
}
