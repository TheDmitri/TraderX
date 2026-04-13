/**
 * Global transaction and menu lifecycle events for TraderX.
 * External mods can subscribe to these events without modifying TraderX code.
 *
 * Usage:
 *   TraderXTransactionEvents.Event_OnAfterTransaction.Insert(MyHandler);
 *
 * Handler signature varies per event (see comments below).
 */
class TraderXTransactionEvents
{
    /**
     * Fired BEFORE payment is processed in a transaction.
     * Handler: void OnBeforeTransaction(PlayerBase player, TraderXTransaction transaction, int calculatedPrice)
     */
    static ref ScriptInvoker Event_OnBeforeTransaction = new ScriptInvoker();

    /**
     * Fired AFTER a transaction completes (success or failure).
     * Handler: void OnAfterTransaction(PlayerBase player, TraderXTransactionResult result)
     */
    static ref ScriptInvoker Event_OnAfterTransaction = new ScriptInvoker();

    /**
     * Fired when the trader menu is opened for a specific NPC (client-side).
     * Handler: void OnTraderMenuOpened(int npcId)
     */
    static ref ScriptInvoker Event_OnTraderMenuOpened = new ScriptInvoker();

    /**
     * Fired when the trader menu is closed (client-side).
     * Handler: void OnTraderMenuClosed(int npcId)
     */
    static ref ScriptInvoker Event_OnTraderMenuClosed = new ScriptInvoker();

    /**
     * Fired when an insurance policy is created for a vehicle.
     * Handler: void OnInsurancePolicyCreated(PlayerBase player, TraderXInsurancePolicy policy)
     */
    static ref ScriptInvoker Event_OnInsurancePolicyCreated = new ScriptInvoker();

    /**
     * Fired when a vehicle receipt is issued.
     * Handler: void OnReceiptIssued(PlayerBase player, TraderXReceipt receipt)
     */
    static ref ScriptInvoker Event_OnReceiptIssued = new ScriptInvoker();
}
