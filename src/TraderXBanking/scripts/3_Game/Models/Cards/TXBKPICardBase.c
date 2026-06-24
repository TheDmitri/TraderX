// TraderXBanking - KPI Card Base Class
// Abstract base class for all KPI cards (Balance, Vault, Clan, Stats).
// Provides common properties and interface for the Home Tab dashboard.

class TXBKPICardBase
{
    // Card identity
    string label;           // "PERSONAL BALANCE", "PERSONAL VAULT", etc.
    string badge;           // Pill text: "+500 today", "Box #47", "Officer", "All flows"
    
    // Main value display
    string valueText;       // Large text: "14,165", "42 / 100", "12 members"
    string subText;         // Context: "Roubles - 28.3% of 50k cap", "slots - next fee 03h 12m"
    
    // Progress bar (0.0 to 1.0, -1 if no progress)
    float progressRatio;
    
    // Footer
    string footerLeft;      // "On hand: 1,240", "-126 R/day", "+4,000 contrib. 30d"
    string footerRight;     // "OPEN >" navigation label
    
    // Navigation target for the "OPEN" button
    string navTarget;       // "banking-account-tab", "banking-vault-tab", etc.
    
    // State flags
    bool isEmpty;           // True if no data available yet
    string emptyMessage;    // Message to show when empty: "No vault configured"
    
    void TXBKPICardBase()
    {
        label = "";
        badge = "";
        valueText = "-";
        subText = "";
        progressRatio = -1.0;
        footerLeft = "";
        footerRight = "OPEN >";
        navTarget = "";
        isEmpty = false;
        emptyMessage = "";
    }
    
    // Check if this card has a progress bar
    bool HasProgress()
    {
        return progressRatio >= 0.0;
    }
    
    // Get formatted progress percentage for display
    string GetProgressPercent()
    {
        if (!HasProgress()) return "";
        return TXBFormat.Percentage(progressRatio);
    }
    
    // Get the main value color class (for styling)
    string GetValueColorClass()
    {
        return "dz-tc-D8D2C0";  // Default ivory
    }
    
    // Get badge color class
    string GetBadgeColorClass()
    {
        return "dz-tc-9A9488";  // Default ash
    }
}
