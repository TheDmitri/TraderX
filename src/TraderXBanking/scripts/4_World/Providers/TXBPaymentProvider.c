/**
 * TraderXBanking implementation of ITraderXPaymentProvider.
 * Bridges the TraderX provider interface to the existing TXBBankingService.
 *
 * Registered automatically when TraderXBanking module loads.
 * If a server owner uses a different banking mod, they implement their own
 * ITraderXPaymentProvider and register it instead.
 */
class TXBPaymentProvider : ITraderXPaymentProvider
{
    override int GetBalance(PlayerBase player)
    {
        if (!GetGame().IsServer())
            return GetClientBalance();

        ref TXBAccountData account = GetTXBBankingService().GetOrCreateAccount(player);
        if (!account)
            return 0;

        return account.balance;
    }

    override int GetClientBalance()
    {
        TXBAccountData clientAccount = GetTXBBankingService().GetClientAccount();
        if (!clientAccount)
            return 0;

        return clientAccount.balance;
    }

    override bool Deduct(PlayerBase player, int amount)
    {
        if (!GetGame().IsServer())
            return false;

        ref TXBAccountData account = GetTXBBankingService().GetOrCreateAccount(player);
        if (!account)
            return false;

        if (account.balance < amount)
            return false;

        account.balance -= amount;
        account.Save();
        GetTXBBankingService().SendAccountToClient(player, account);

        TXBSettings settings = GetTXBConfigService().GetServerSettings();
        if (settings && settings.enableLogs)
            DZLog().LogInfo("[TXB] Provider deduct: " + player.GetIdentity().GetName() + " amount: " + amount + " | balance: " + account.balance);

        return true;
    }

    override void Credit(PlayerBase player, int amount)
    {
        if (!GetGame().IsServer())
            return;

        ref TXBAccountData account = GetTXBBankingService().GetOrCreateAccount(player);
        if (!account)
            return;

        account.balance = Math.Min(account.balance + amount, account.maxBalance);
        account.Save();
        GetTXBBankingService().SendAccountToClient(player, account);

        TXBSettings settings = GetTXBConfigService().GetServerSettings();
        if (settings && settings.enableLogs)
            DZLog().LogInfo("[TXB] Provider credit: " + player.GetIdentity().GetName() + " amount: " + amount + " | balance: " + account.balance);
    }

    override string GetDisplayName()
    {
        return "Bank";
    }

    override bool IsAvailable(PlayerBase player)
    {
        if (!player)
            return false;

        // Check credit card requirement if configured
        TXBSettings settings;
        if (GetGame().IsServer())
            settings = GetTXBConfigService().GetServerSettings();
        else
            settings = GetTXBConfigService().GetClientSettings();

        if (settings && settings.requireCreditCard)
        {
            ItemBase inHands = player.GetItemInHands();
            if (!inHands || !inHands.IsKindOf("TXBCreditCard"))
                return false;
        }

        return true;
    }

    override void RequestBalanceRefresh()
    {
        // Client-side: send RPC to request fresh account data
        if (GetGame().IsClient() || !GetGame().IsMultiplayer())
        {
            GetDayZularRPCManager().SendRPC(
                TXB_RPC_NAMESPACE,
                "RequestAccount",
                null,
                true,
                null
            );
        }
    }
}
