// TraderXBanking - Ledger entry type enum
// Broad categorisation of what moved money in/out of an account.
// Stored as int in TXBLedgerEntry.type for JSON compactness and forward compat.

class TXBLedgerEntryType
{
	static const int DEPOSIT        = 0;
	static const int WITHDRAW       = 1;
	static const int TRANSFER_IN    = 2;
	static const int TRANSFER_OUT   = 3;
	static const int TRADER_BUY     = 4;
	static const int TRADER_SELL    = 5;
	static const int GARAGE_FEE_IN  = 6;
	static const int GARAGE_FEE_OUT = 7;
	static const int VAULT_FEE_OUT  = 8;

	static string GetLabel(int t)
	{
		switch (t)
		{
			case DEPOSIT:        return "Deposit";
			case WITHDRAW:       return "Withdraw";
			case TRANSFER_IN:    return "Transfer in";
			case TRANSFER_OUT:   return "Transfer out";
			case TRADER_BUY:     return "Purchase";
			case TRADER_SELL:    return "Sell income";
			case GARAGE_FEE_IN:  return "Garage refund";
			case GARAGE_FEE_OUT: return "Garage fee";
			case VAULT_FEE_OUT:  return "Vault fee";
		}
		return "Unknown";
	}

	static bool IsInflow(int t)
	{
		return t == DEPOSIT || t == TRANSFER_IN || t == TRADER_SELL || t == GARAGE_FEE_IN;
	}
}
