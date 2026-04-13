/**
 * Centralized payment service that routes payments through the appropriate provider
 * based on server configuration (paymentPriority) and provider availability.
 *
 * This service sits between the transaction service and the actual payment mechanisms
 * (inventory currency and/or banking provider).
 */
class TraderXPaymentService
{
    private static ref TraderXPaymentService m_Instance;

    static TraderXPaymentService GetInstance()
    {
        if (!m_Instance)
            m_Instance = new TraderXPaymentService();
        return m_Instance;
    }

    /**
     * Get the total money available to a player (cash + bank if available).
     * Server-side.
     */
    int GetPlayerTotalMoney(PlayerBase player, ref TStringArray acceptedCurrencyTypes = null)
    {
        int cash = TraderXCurrencyService.GetInstance().GetPlayerMoneyFromAllCurrency(player, acceptedCurrencyTypes);
        int bank = 0;

        if (TraderXProviderRegistry.GetInstance().HasBankingProvider())
        {
            ITraderXPaymentProvider bankProvider = TraderXProviderRegistry.GetInstance().GetBankingProvider();
            if (bankProvider.IsAvailable(player))
                bank = bankProvider.GetBalance(player);
        }

        return cash + bank;
    }

    /**
     * Get total money on client side (for UI display).
     */
    int GetPlayerTotalMoneyClient(PlayerBase player, ref TStringArray acceptedCurrencyTypes = null)
    {
        int cash = TraderXCurrencyService.GetInstance().GetPlayerMoneyFromAllCurrency(player, acceptedCurrencyTypes);
        int bank = 0;

        if (TraderXProviderRegistry.GetInstance().HasBankingProvider())
            bank = TraderXProviderRegistry.GetInstance().GetBankingProvider().GetClientBalance();

        return cash + bank;
    }

    /**
     * Remove money from the player using the configured payment priority.
     * Server-side only.
     *
     * @param player The player to charge
     * @param amount The total amount to remove
     * @param acceptedCurrencyTypes Currency types accepted by the NPC
     * @param paymentPriority The payment priority setting
     * @return true if the full amount was successfully removed
     */
    bool RemovePlayerMoney(PlayerBase player, int amount, ref TStringArray acceptedCurrencyTypes, string paymentPriority)
    {
        if (amount <= 0)
            return true;

        if (paymentPriority == "cash_only" || !TraderXProviderRegistry.GetInstance().HasBankingProvider())
        {
            return TraderXCurrencyService.GetInstance().RemoveMoneyAmountFromPlayer(player, amount, acceptedCurrencyTypes);
        }

        ITraderXPaymentProvider bankProvider = TraderXProviderRegistry.GetInstance().GetBankingProvider();

        if (paymentPriority == "bank_only")
        {
            if (!bankProvider.IsAvailable(player))
                return false;
            return bankProvider.Deduct(player, amount);
        }

        if (paymentPriority == "bank_first")
        {
            return RemoveBankFirst(player, amount, acceptedCurrencyTypes, bankProvider);
        }

        // Default: cash_first
        return RemoveCashFirst(player, amount, acceptedCurrencyTypes, bankProvider);
    }

    /**
     * Add money to the player using the configured payment priority.
     * Server-side only.
     */
    void AddPlayerMoney(PlayerBase player, int amount, ref TStringArray acceptedCurrencyTypes, string paymentPriority)
    {
        if (amount <= 0)
            return;

        if (paymentPriority == "bank_only" && TraderXProviderRegistry.GetInstance().HasBankingProvider())
        {
            ITraderXPaymentProvider bankProvider = TraderXProviderRegistry.GetInstance().GetBankingProvider();
            if (bankProvider.IsAvailable(player))
            {
                bankProvider.Credit(player, amount);
                return;
            }
        }

        if (paymentPriority == "bank_first" && TraderXProviderRegistry.GetInstance().HasBankingProvider())
        {
            ITraderXPaymentProvider bankProvider = TraderXProviderRegistry.GetInstance().GetBankingProvider();
            if (bankProvider.IsAvailable(player))
            {
                bankProvider.Credit(player, amount);
                return;
            }
        }

        // cash_first or cash_only or no bank available: add as inventory currency
        TraderXCurrencyService.GetInstance().AddMoneyToPlayer(player, amount, acceptedCurrencyTypes);
    }

    /**
     * Try cash first, then use bank for the remainder.
     */
    private bool RemoveCashFirst(PlayerBase player, int amount, ref TStringArray acceptedCurrencyTypes, ITraderXPaymentProvider bankProvider)
    {
        int cashAvailable = TraderXCurrencyService.GetInstance().GetPlayerMoneyFromAllCurrency(player, acceptedCurrencyTypes);

        if (cashAvailable >= amount)
        {
            return TraderXCurrencyService.GetInstance().RemoveMoneyAmountFromPlayer(player, amount, acceptedCurrencyTypes);
        }

        // Not enough cash — try split: use all cash + bank for remainder
        if (!bankProvider.IsAvailable(player))
            return false;

        int bankBalance = bankProvider.GetBalance(player);
        int remainder = amount - cashAvailable;

        if (bankBalance < remainder)
            return false;

        // Remove all cash first
        if (cashAvailable > 0)
        {
            if (!TraderXCurrencyService.GetInstance().RemoveMoneyAmountFromPlayer(player, cashAvailable, acceptedCurrencyTypes))
                return false;
        }

        // Deduct remainder from bank
        if (!bankProvider.Deduct(player, remainder))
        {
            // Rollback: give cash back
            if (cashAvailable > 0)
                TraderXCurrencyService.GetInstance().AddMoneyToPlayer(player, cashAvailable, acceptedCurrencyTypes);
            return false;
        }

        return true;
    }

    /**
     * Try bank first, then use cash for the remainder.
     */
    private bool RemoveBankFirst(PlayerBase player, int amount, ref TStringArray acceptedCurrencyTypes, ITraderXPaymentProvider bankProvider)
    {
        if (!bankProvider.IsAvailable(player))
        {
            // Bank not available, fall back to cash
            return TraderXCurrencyService.GetInstance().RemoveMoneyAmountFromPlayer(player, amount, acceptedCurrencyTypes);
        }

        int bankBalance = bankProvider.GetBalance(player);

        if (bankBalance >= amount)
        {
            return bankProvider.Deduct(player, amount);
        }

        // Not enough in bank — try split: use all bank + cash for remainder
        int cashAvailable = TraderXCurrencyService.GetInstance().GetPlayerMoneyFromAllCurrency(player, acceptedCurrencyTypes);
        int remainder = amount - bankBalance;

        if (cashAvailable < remainder)
            return false;

        // Deduct all from bank first
        if (bankBalance > 0)
        {
            if (!bankProvider.Deduct(player, bankBalance))
                return false;
        }

        // Remove remainder from cash
        if (!TraderXCurrencyService.GetInstance().RemoveMoneyAmountFromPlayer(player, remainder, acceptedCurrencyTypes))
        {
            // Rollback: credit bank back
            if (bankBalance > 0)
                bankProvider.Credit(player, bankBalance);
            return false;
        }

        return true;
    }
}
