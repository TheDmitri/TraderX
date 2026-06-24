// TraderXBanking - Recent Activity ViewModel
// Manages the list of recent ledger entries for the Home Tab.
// Handles slicing, empty states, and formatting for display.

class TXBRecentActivityViewModel
{
    // Source data
    ref array<ref TXBLedgerEntry> allEntries;
    
    // Display settings
    int maxVisible;
    
    // Computed
    bool isEmpty;
    string emptyMessage;
    ref array<ref TXBLedgerEntry> visibleEntries;
    
    void TXBRecentActivityViewModel()
    {
        allEntries = new array<ref TXBLedgerEntry>();
        visibleEntries = new array<ref TXBLedgerEntry>();
        maxVisible = 6;
        isEmpty = true;
        emptyMessage = "No recent activity";
    }
    
    // Factory: Create from ledger cache
    static TXBRecentActivityViewModel FromLedger(array<ref TXBLedgerEntry> ledgerEntries, int maxToShow)
    {
        TXBRecentActivityViewModel vm = new TXBRecentActivityViewModel();
        vm.maxVisible = maxToShow;
        
        if (ledgerEntries && ledgerEntries.Count() > 0)
        {
            vm.allEntries = ledgerEntries;
            vm.isEmpty = false;
            vm.ComputeVisible();
        }
        else
        {
            vm.isEmpty = true;
            vm.visibleEntries.Clear();
        }
        
        return vm;
    }
    
    // Factory: Empty state
    static TXBRecentActivityViewModel Empty()
    {
        TXBRecentActivityViewModel vm = new TXBRecentActivityViewModel();
        vm.isEmpty = true;
        vm.emptyMessage = "No recent activity";
        return vm;
    }
    
    // Compute the visible slice
    protected void ComputeVisible()
    {
        visibleEntries.Clear();
        
        int count = allEntries.Count();
        int toShow = Math.Min(count, maxVisible);
        
        for (int i = 0; i < toShow; i++)
        {
            visibleEntries.Insert(allEntries[i]);
        }
        
        isEmpty = visibleEntries.Count() == 0;
    }
    
    // Update the source data and recompute
    void UpdateEntries(array<ref TXBLedgerEntry> newEntries)
    {
        allEntries = newEntries;
        ComputeVisible();
    }
    
    // Get the visible entries for binding
    array<ref TXBLedgerEntry> GetVisible()
    {
        return visibleEntries;
    }
    
    // Get total count (for "View all X entries" type messages)
    int GetTotalCount()
    {
        return allEntries.Count();
    }
    
    // Check if there are more entries than visible
    bool HasMore()
    {
        return allEntries.Count() > maxVisible;
    }
    
    // Get "View all N" message
    string GetViewAllMessage()
    {
        return "View all " + allEntries.Count().ToString() + " entries >";
    }
    
    // Get empty state message
    string GetEmptyMessage()
    {
        return emptyMessage;
    }
}
