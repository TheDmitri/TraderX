// TraderXBanking - Balance KPI Card
// Displays personal account balance with delta and progress.
// Reusable in: HomeTab (KPI grid), AccountTab (balance card)

class TXBKPIBalanceCard : TXBKPICardBase
{
    // Raw data (for calculations)
    int balance;
    int maxBalance;
    int onHandCash;
    int deltaToday;
    
    void TXBKPIBalanceCard()
    {
        label = "PERSONAL BALANCE";
        navTarget = "banking-account-tab";
    }
    
    // Factory: Create from account data
    static TXBKPIBalanceCard FromAccount(TXBAccountData account, int cashOnHand, int todayDelta)
    {
        TXBKPIBalanceCard card = new TXBKPIBalanceCard();
        
        if (account)
        {
            card.balance = account.balance;
            card.maxBalance = account.maxBalance;
            card.onHandCash = cashOnHand;
            card.deltaToday = todayDelta;
            
            // Format display values
            card.valueText = TXBFormat.Currency(card.balance);
            card.badge = TXBFormat.Delta(todayDelta) + " today";
            card.subText = "Roubles";
            
            // Progress bar (always shown for balance)
            card.progressRatio = TXBFormat.Ratio(card.balance, card.maxBalance);
            
            // Footer
            card.footerLeft = "On hand: " + TXBFormat.Currency(cashOnHand);
        }
        else
        {
            card.isEmpty = true;
            card.emptyMessage = "No account data";
        }
        
        return card;
    }
    
    // Factory: Create simplified version for AccountTab (no delta)
    static TXBKPIBalanceCard FromAccountSimple(TXBAccountData account, int cashOnHand)
    {
        TXBKPIBalanceCard card = FromAccount(account, cashOnHand, 0);
        card.badge = "";
        return card;
    }
    
    // Get badge color based on delta
    override string GetBadgeColorClass()
    {
        if (deltaToday > 0) return "dz-tc-9AB05A";  // Olive for positive
        if (deltaToday < 0) return "dz-tc-C8664A";  // Rust for negative
        return "dz-tc-9A9488";  // Ash for zero
    }
    
    // Get value color (amber for balance)
    override string GetValueColorClass()
    {
        return "dz-tc-D4A44C";  // Amber
    }
    
    // Check if balance is near capacity
    bool IsNearCapacity()
    {
        if (maxBalance <= 0) return false;
        return (balance / (maxBalance * 1.0)) > 0.9;
    }
    
    // Format for compact display (used in small spaces)
    string GetCompactDisplay()
    {
        return TXBFormat.Currency(balance);
    }
}
