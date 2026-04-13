/**
 * Interface for vehicle insurance providers.
 * External mods implement this and register via TraderXProviderRegistry.
 */
class ITraderXInsuranceProvider
{
    bool CanInsure(PlayerBase player, string vehicleType, int vehiclePrice) { return false; }

    int CalculatePremium(int vehiclePrice) { return 0; }

    int CalculatePayout(int vehiclePrice) { return 0; }

    bool CreatePolicy(PlayerBase player, string vehicleType, int vehicleNetworkLow, int vehicleNetworkHigh, int vehiclePrice, string presetId) { return false; }

    bool VoidPolicy(string policyId, string playerUID) { return false; }

    bool ClaimPolicy(PlayerBase player, string policyId, out int payoutAmount) { return false; }

    int GetActivePolicyCount(string playerUID) { return 0; }

    bool IsVehicleInsured(int networkLow, int networkHigh) { return false; }

    void OnVehicleParkedInGarage(string playerUID, int networkLow, int networkHigh) {}

    void OnVehicleParkedOutOfGarage(string playerUID, int networkLow, int networkHigh) {}

    void ScanForMissingVehicles() {}

    string GetDisplayName() { return "Insurance"; }
}
