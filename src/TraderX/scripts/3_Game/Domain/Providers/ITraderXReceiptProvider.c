/**
 * Interface for vehicle receipt providers.
 * External mods implement this and register via TraderXProviderRegistry.
 */
class ITraderXReceiptProvider
{
    bool CreateReceipt(PlayerBase player, string vehicleType, string presetId, ref array<string> presetAttachments, int buyPrice, ref array<int> lockData) { return false; }

    bool RedeemReceipt(PlayerBase player, string receiptId, int traderId, out string errorMsg) { return false; }

    bool IsReceiptValid(string receiptId, string playerUID) { return false; }

    int GetReceiptCount(string playerUID) { return 0; }

    void PurgeExpiredReceipts(string playerUID) {}

    string GetDisplayName() { return "Receipts"; }
}
