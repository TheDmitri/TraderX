// TraderXBanking - Stats KPI Card
// Displays 30-day financial statistics (inflow, outflow, transfer count).
// Used in: HomeTab KPI grid.

class TXBKPIStatsCard : TXBKPICardBase
{
    // Raw data
    int inflow30;
    int outflow30;
    int transferCount;
    int totalTx;
    
    void TXBKPIStatsCard()
    {
        label = "STATS · 30 DAYS";
        badge = "All flows";
        navTarget = "txb-ledger-tab";
        progressRatio = -1.0;
        footerRight = "VIEW LEDGER →";
    }
    
    // Factory: Create from computed stats
    static TXBKPIStatsCard FromStats(int inflow, int outflow, int transfers, int totalTransactions)
    {
        TXBKPIStatsCard card = new TXBKPIStatsCard();
        
        card.inflow30 = inflow;
        card.outflow30 = outflow;
        card.transferCount = transfers;
        card.totalTx = totalTransactions;
        
        // Format display values
        card.valueText = card.FormatValueDisplay();
        card.subText = TXBFormat.Signed(inflow) + " in · " + TXBFormat.Signed(outflow) + " out";
        card.footerLeft = transfers.ToString() + " transfers · " + totalTransactions.ToString() + " tx";
        
        return card;
    }
    
    // Factory: Create empty state
    static TXBKPIStatsCard Empty()
    {
        TXBKPIStatsCard card = new TXBKPIStatsCard();
        card.valueText = FormatInOutDisplay(0, 0);
        card.footerLeft = "0 transfers · 0 tx";
        return card;
    }
    
    // Format main value display (in/out side by side)
    string FormatValueDisplay()
    {
        return FormatInOutDisplay(inflow30, outflow30);
    }
    
    // Static helper for in/out formatting
    static string FormatInOutDisplay(int inflow, int outflow)
    {
        CurrencyPipe currencyPipe = new CurrencyPipe();
        
        string inStr = currencyPipe.Transform(inflow.ToString(), null);
        string outStr = currencyPipe.Transform(outflow.ToString(), null);
        
        return "+" + inStr + " · -" + outStr;
    }
    
    // Get color for inflow value
    string GetInflowColor()
    {
        return "dz-tc-9AB05A";
    }
    
    // Get color for outflow value
    string GetOutflowColor()
    {
        return "dz-tc-C8664A";
    }
    
    // Check if stats show activity
    bool HasActivity()
    {
        return inflow30 > 0 || outflow30 > 0;
    }
}
