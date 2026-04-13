/**
 * Abstract payment provider interface for TraderX.
 * External mods (e.g., TraderXBanking) extend this class and register
 * via TraderXProviderRegistry to enable bank-based payments in the trader.
 *
 * If no provider is registered, TraderX falls back to inventory currency only.
 */
class ITraderXPaymentProvider
{
    /**
     * Get the player's balance from this payment provider (server-side).
     * @param player The player to check balance for
     * @return Current balance amount
     */
    int GetBalance(PlayerBase player)
    {
        return 0;
    }

    /**
     * Get the player's cached balance on client side (for UI display).
     * @return Cached client-side balance
     */
    int GetClientBalance()
    {
        return 0;
    }

    /**
     * Deduct amount from the provider. Server-side only.
     * @param player The player to deduct from
     * @param amount The amount to deduct
     * @return true if deduction was successful, false if insufficient funds
     */
    bool Deduct(PlayerBase player, int amount)
    {
        return false;
    }

    /**
     * Credit amount to the provider. Server-side only.
     * @param player The player to credit
     * @param amount The amount to credit
     */
    void Credit(PlayerBase player, int amount)
    {
    }

    /**
     * Display name for UI labels (e.g., "Bank").
     */
    string GetDisplayName()
    {
        return "";
    }

    /**
     * Whether this provider is currently available for the given player.
     * Can check for required items (e.g., credit card), account status, etc.
     * @param player The player to check availability for
     */
    bool IsAvailable(PlayerBase player)
    {
        return false;
    }

    /**
     * Request a fresh balance sync from server to client.
     * Called when the trader menu opens so the UI has up-to-date data.
     */
    void RequestBalanceRefresh()
    {
    }
}
