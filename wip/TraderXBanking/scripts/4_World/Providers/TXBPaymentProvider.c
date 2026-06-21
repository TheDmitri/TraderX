/**
 * TraderXBanking implementation of ITraderXPaymentProvider.
 * Bridges the TraderX provider interface to the existing TXBBankingService.
 *
 * Registered automatically when TraderXBanking module loads.
 * If a server owner uses a different banking mod, they implement their own
 * ITraderXPaymentProvider and register it instead.
 *
 * NOTE: Only compiled when TraderX is present (requires ITraderXPaymentProvider interface).
 */
#ifdef TraderX
class TXBPaymentProvider : ITraderXPaymentProvider
{
    override int GetBalance(Man player)
    {
        if (!GetGame().IsServer())
            return GetClientBalance();

        PlayerBase pb = PlayerBase.Cast(player);
        if (!pb)
            return 0;

        TXBAccountData account = GetTXBBankingService().GetOrCreateAccount(pb);
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

    override bool Deduct(Man player, int amount)
    {
        if (!GetGame().IsServer())
            return false;

        PlayerBase pb = PlayerBase.Cast(player);
        if (!pb)
            return false;

        TXBAccountData account = GetTXBBankingService().GetOrCreateAccount(pb);
        if (!account)
            return false;

        if (account.balance < amount)
            return false;

        account.balance -= amount;
        account.Save();
        GetTXBBankingService().SendAccountToClient(pb, account);

        // Ledger: Trader purchase — buyer's account loses money.
        string uid = pb.GetIdentity().GetPlainId();
        string playerName = pb.GetIdentity().GetName();
        GetTXBBankingService().RecordLedgerEntry(uid, TXBLedgerEntryType.TRADER_BUY, amount, 0, account.balance, "Trader", "", "", "");

        TXBSettings settings = GetTXBConfigService().GetServerSettings();
        if (settings && settings.enableLogs)
            DZLog().LogInfo("[TXB] Provider deduct: " + playerName + " amount: " + amount + " | balance: " + account.balance);

        return true;
    }

    override void Credit(Man player, int amount)
    {
        if (!GetGame().IsServer())
            return;

        PlayerBase pb = PlayerBase.Cast(player);
        if (!pb)
            return;

        TXBAccountData account = GetTXBBankingService().GetOrCreateAccount(pb);
        if (!account)
            return;

        account.balance = Math.Min(account.balance + amount, account.maxBalance);
        account.Save();
        GetTXBBankingService().SendAccountToClient(pb, account);

        // Ledger: Trader sell-back — seller's account gains money.
        string uid = pb.GetIdentity().GetPlainId();
        string playerName = pb.GetIdentity().GetName();
        GetTXBBankingService().RecordLedgerEntry(uid, TXBLedgerEntryType.TRADER_SELL, amount, 0, account.balance, "Trader", "", "", "");

        TXBSettings settings = GetTXBConfigService().GetServerSettings();
        if (settings && settings.enableLogs)
            DZLog().LogInfo("[TXB] Provider credit: " + playerName + " amount: " + amount + " | balance: " + account.balance);
    }

    override string GetDisplayName()
    {
        return "Bank";
    }

    override bool IsAvailable(Man player)
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
            PlayerBase pb = PlayerBase.Cast(player);
            if (!pb)
                return false;
            ItemBase inHands = pb.GetItemInHands();
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
            GetDayZularRPCManager().SendRPC( TXBConstants.RPC_NAMESPACE, "RequestAccount", null, true, null );
        }
    }

    // Labeled variants — caller passes a category string (e.g. "GARAGE_FEE_OUT"),
    // which we map to a TXBLedgerEntryType and log against the player's account.
    // Avoids compile-time cross-mod refs because Garage only knows the label string.
    override bool DeductLabeled(Man player, int amount, string category, string counterpartyName)
    {
        if (!GetGame().IsServer()) return false;

        PlayerBase pb = PlayerBase.Cast(player);
        if (!pb) return false;

        TXBAccountData account = GetTXBBankingService().GetOrCreateAccount(pb);
        if (!account) return false;

        if (account.balance < amount)
            return false;

        account.balance -= amount;
        account.Save();
        GetTXBBankingService().SendAccountToClient(pb, account);

        string uid = pb.GetIdentity().GetPlainId();
        int entryType = MapCategoryToEntryType(category, true);
        string cparty = counterpartyName;
        if (cparty == "") cparty = CategoryToCounterparty(category);
        GetTXBBankingService().RecordLedgerEntry(uid, entryType, amount, 0, account.balance, cparty, "", "", "");
        return true;
    }

    override void CreditLabeled(Man player, int amount, string category, string counterpartyName)
    {
        if (!GetGame().IsServer()) return;

        PlayerBase pb = PlayerBase.Cast(player);
        if (!pb) return;

        TXBAccountData account = GetTXBBankingService().GetOrCreateAccount(pb);
        if (!account) return;

        account.balance = Math.Min(account.balance + amount, account.maxBalance);
        account.Save();
        GetTXBBankingService().SendAccountToClient(pb, account);

        string uid = pb.GetIdentity().GetPlainId();
        int entryType = MapCategoryToEntryType(category, false);
        string cparty = counterpartyName;
        if (cparty == "") cparty = CategoryToCounterparty(category);
        GetTXBBankingService().RecordLedgerEntry(uid, entryType, amount, 0, account.balance, cparty, "", "", "");
    }

    protected int MapCategoryToEntryType(string category, bool isDeduct)
    {
        if (category == "GARAGE_FEE_OUT") return TXBLedgerEntryType.GARAGE_FEE_OUT;
        if (category == "GARAGE_FEE_IN")  return TXBLedgerEntryType.GARAGE_FEE_IN;
        if (category == "TRADER_BUY")     return TXBLedgerEntryType.TRADER_BUY;
        if (category == "TRADER_SELL")    return TXBLedgerEntryType.TRADER_SELL;
        if (isDeduct) return TXBLedgerEntryType.TRADER_BUY;
        return TXBLedgerEntryType.TRADER_SELL;
    }

    protected string CategoryToCounterparty(string category)
    {
        if (category == "GARAGE_FEE_OUT" || category == "GARAGE_FEE_IN") return "Garage";
        return "Trader";
    }
}
#endif // TraderX
