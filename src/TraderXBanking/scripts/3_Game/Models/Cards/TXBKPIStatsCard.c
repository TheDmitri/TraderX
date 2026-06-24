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
    
    // Clean per-figure display strings
    string inDisplay;
    string outDisplay;
    string netDisplay;

    void TXBKPIStatsCard()
    {
        label = "STATS - 30 DAYS";
        badge = "All flows";
        navTarget = "txb-ledger-tab";
        progressRatio = -1.0;
        footerRight = "VIEW LEDGER >";
        inDisplay = "+0";
        outDisplay = "-0";
        netDisplay = "+0";
    }

    // Factory: Create from computed stats
    static TXBKPIStatsCard FromStats(int inflow, int outflow, int transfers, int totalTransactions)
    {
        TXBKPIStatsCard card = new TXBKPIStatsCard();

        card.inflow30 = inflow;
        card.outflow30 = outflow;
        card.transferCount = transfers;
        card.totalTx = totalTransactions;

        card.inDisplay = "+" + TXBFormat.Currency(inflow);
        card.outDisplay = "-" + TXBFormat.Currency(outflow);
        card.netDisplay = TXBFormat.Delta(inflow - outflow);

        card.valueText = card.netDisplay;
        card.subText = "Net 30 days";
        card.footerLeft = transfers.ToString() + " transfers - " + totalTransactions.ToString() + " tx";

        return card;
    }

    // Factory: Create empty state
    static TXBKPIStatsCard Empty()
    {
        TXBKPIStatsCard card = new TXBKPIStatsCard();
        card.valueText = "+0";
        card.subText = "Net 30 days";
        card.footerLeft = "0 transfers - 0 tx";
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
        string inStr = TXBFormat.Currency(inflow);
        string outStr = TXBFormat.Currency(outflow);

        return "+" + inStr + " - -" + outStr;
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
