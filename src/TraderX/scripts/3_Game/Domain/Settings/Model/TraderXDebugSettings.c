/**
 * TraderXDebugSettings - Configuration for debug mode and test suite execution
 * Controls debug mode activation, test suite execution, and test case generation
 */
class TraderXDebugSettings
{
    string version = TRADERX_CURRENT_VERSION;
    
    // Debug Mode Settings
    bool enableDebugMode = false;
    
    // Test Suite Execution Settings
    bool runTestsOnStartup = false;
    bool runCurrencyServiceTests = false;
    bool runTransactionServiceTests = true;
    bool runVehicleTransactionTests = false;
    bool runPricingServiceTests = true;
    bool runJSONTestCases = true;
    
    // Admin Control Settings
    ref array<string> authorizedAdmins = new array<string>();
    
    void TraderXDebugSettings()
    {
        // Constructor - default values already set above
    }
    
    /**
     * Create default debug settings
     */
    void DefaultTraderXDebugSettings()
    {
        enableDebugMode = false;
        runTestsOnStartup = false;
        runCurrencyServiceTests = false;
        runTransactionServiceTests = true;
        runVehicleTransactionTests = false;
        runPricingServiceTests = true;
        runJSONTestCases = true;
        
        // Initialize with empty admin list - add Steam64 IDs as needed
        authorizedAdmins.Clear();
        authorizedAdmins.Insert("76561198047475641");
    }
    
    /**
     * Validate settings and apply constraints
     */
    void ValidateSettings()
    {
        // No validation needed for current properties
    }
    
    /**
     * Check if any test suite should run
     */
    bool ShouldRunAnyTests()
    {
        return runCurrencyServiceTests || runTransactionServiceTests || runVehicleTransactionTests || runPricingServiceTests || runJSONTestCases;
    }
    
    /**
     * Check if a player is authorized to trigger debug features
     * @param playerId Steam64 ID of the player
     * @return true if player is authorized admin
     */
    bool IsAuthorizedAdmin(string playerId)
    {
        if (!authorizedAdmins || authorizedAdmins.Count() == 0)
            return false;
            
        for (int i = 0; i < authorizedAdmins.Count(); i++)
        {
            if (authorizedAdmins[i] == playerId)
                return true;
        }
        
        return false;
    }
    
    /**
     * Add an admin to the authorized list
     * @param playerId Steam64 ID to add
     */
    void AddAuthorizedAdmin(string playerId)
    {
        if (!authorizedAdmins)
            authorizedAdmins = new array<string>();
            
        if (!IsAuthorizedAdmin(playerId))
            authorizedAdmins.Insert(playerId);
    }
    
    /**
     * Remove an admin from the authorized list
     * @param playerId Steam64 ID to remove
     */
    void RemoveAuthorizedAdmin(string playerId)
    {
        if (!authorizedAdmins)
            return;
            
        int index = authorizedAdmins.Find(playerId);
        if (index != -1)
            authorizedAdmins.Remove(index);
    }
    
    /**
     * Get formatted settings summary for logging
     */
    string GetSettingsSummary()
    {
        string summary = "[DEBUG SETTINGS]\n";
        
        if (enableDebugMode)
            summary += string.Format("  Debug Mode: ENABLED\n");
        else
            summary += string.Format("  Debug Mode: DISABLED\n");
        
        if (runTestsOnStartup)
            summary += string.Format("  Run Tests on Startup: YES\n");
        else
            summary += string.Format("  Run Tests on Startup: NO\n");
        
        summary += string.Format("  Test Suites Enabled:\n");
        
        if (runCurrencyServiceTests)
            summary += string.Format("    - Currency Service: YES\n");
        else
            summary += string.Format("    - Currency Service: NO\n");
        
        if (runTransactionServiceTests)
            summary += string.Format("    - Transaction Service: YES\n");
        else
            summary += string.Format("    - Transaction Service: NO\n");
        
        if (runPricingServiceTests)
            summary += string.Format("    - Pricing Service: YES\n");
        else
            summary += string.Format("    - Pricing Service: NO\n");
        
        if (runVehicleTransactionTests)
            summary += string.Format("    - Vehicle Transactions: YES\n");
        else
            summary += string.Format("    - Vehicle Transactions: NO\n");
        
        if (runJSONTestCases)
            summary += string.Format("    - JSON Test Cases: YES\n");
        else
            summary += string.Format("    - JSON Test Cases: NO\n");
        
        if(authorizedAdmins && authorizedAdmins.Count() > 0)
        {
            summary += string.Format("  Authorized Admins: %1\n", authorizedAdmins.Count().ToString());
            for (int i = 0; i < authorizedAdmins.Count(); i++)
            {
                summary += string.Format("    - %1\n", authorizedAdmins[i]);
            }
        }
        
        return summary;
    }
}
