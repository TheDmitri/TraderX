/**
 * TraderXDebugModeService - Manages debug mode and test case generation
 * Captures real transaction scenarios for automated test case creation
 */
class TraderXDebugModeService
{
    static ref TraderXDebugModeService m_instance;
    
    bool m_debugModeEnabled = false;
    
    ref array<ref TraderXTransactionTestCase> m_capturedTestCases;
    ref TraderXTransactionTestCase m_currentTestCase;
    
    void TraderXDebugModeService()
    {
        m_capturedTestCases = new array<ref TraderXTransactionTestCase>();
    }
    
    static TraderXDebugModeService GetInstance()
    {
        if (!m_instance)
            m_instance = new TraderXDebugModeService();
        return m_instance;
    }
    
    void EnableDebugMode(bool enable = true)
    {
        m_debugModeEnabled = enable;
        if(enable)
        {
            GetTraderXLogger().LogDebug("[DEBUG MODE] ENABLED");
        }
        else
        {
            GetTraderXLogger().LogDebug("[DEBUG MODE] DISABLED");
        }
        
        if (enable)
        {
            GetTraderXLogger().LogDebug("[DEBUG MODE] Transaction test cases will be automatically generated");
        }
    }
    
    bool IsDebugModeEnabled()
    {
        return m_debugModeEnabled;
    }
    
    void BeginTransactionCapture(PlayerBase player, TraderXTransactionCollection transactions, int traderId, string description = "Auto-generated test case")
    {
        StartTransactionCapture(player, transactions, traderId, description);
    }
    
    void StartTransactionCapture(PlayerBase player, TraderXTransactionCollection transactions, int traderId, string description = "Auto-generated test case")
    {
        if (!m_debugModeEnabled || !player || !transactions)
            return;
            
        m_currentTestCase = TraderXTransactionTestCase.Create(description, true);
        m_currentTestCase.CapturePreTransactionState(player, transactions, traderId);
        
        GetTraderXLogger().LogDebug(string.Format("[DEBUG MODE] Started capturing test case: %1", m_currentTestCase.testCaseId));
    }
    
    void CompleteTransactionCapture(PlayerBase player, TraderXTransactionResultCollection result, bool wasSuccessExpected = true)
    {
        if (!m_debugModeEnabled || !m_currentTestCase || !player || !result)
            return;
            
        m_currentTestCase.CapturePostTransactionState(player, result);
        m_currentTestCase.isSuccessExpected = wasSuccessExpected;
        
        // Determine if result matches expectations
        bool actualSuccess = !result.HasErrors();
        bool expectationMet = (actualSuccess == wasSuccessExpected);
        
        if (!expectationMet)
        {
            GetTraderXLogger().LogWarning(string.Format("[DEBUG MODE] Test case captured - EXPECTATION MISMATCH! Expected: %1, Actual: %2", 
                wasSuccessExpected, actualSuccess));
        }
        
        // Save the test case
        m_capturedTestCases.Insert(m_currentTestCase);
        SaveTestCaseToFile(m_currentTestCase);
        
        GetTraderXLogger().LogDebug(string.Format("[DEBUG MODE] Completed test case capture: %1 (Total cases: %2)", 
            m_currentTestCase.testCaseId, m_capturedTestCases.Count()));
        
        m_currentTestCase = null;
    }
    
    void CaptureFailedTransaction(PlayerBase player, TraderXTransactionCollection transactions, int traderId, string failureReason, string description = "Failed transaction test case")
    {
        if (!m_debugModeEnabled)
            return;
            
        TraderXTransactionTestCase testCase = TraderXTransactionTestCase.Create(description, false);
        testCase.CapturePreTransactionState(player, transactions, traderId);
        testCase.expectedFailureReason = failureReason;
        
        // Create a failure result for post-state
        array<ref TraderXTransactionResult> failureResults = new array<ref TraderXTransactionResult>();
        TraderXTransaction firstTransaction = transactions.GetAllTransactions().Get(0);
        TraderXTransactionResult failureResult = TraderXTransactionResult.CreateFailure(firstTransaction.GetTransactionId(), firstTransaction.GetProductId(), firstTransaction.GetTransactionType(), failureReason);
        failureResults.Insert(failureResult);
        
        TraderXTransactionResultCollection resultCollection = TraderXTransactionResultCollection.Create(player.GetIdentity().GetPlainId(), failureResults);
        
        testCase.CapturePostTransactionState(player, resultCollection);
        
        m_capturedTestCases.Insert(testCase);
        SaveTestCaseToFile(testCase);
        
        GetTraderXLogger().LogDebug(string.Format("[DEBUG MODE] Captured failed transaction test case: %1", testCase.testCaseId));
    }
    
    private void SaveTestCaseToFile(TraderXTransactionTestCase testCase)
    {
        if (!testCase)
            return;
            
        string safeTestCaseId = testCase.testCaseId;
        string filePath = string.Format(TRADERX_DEBUG_TEST_CASE_FILE, safeTestCaseId);
        
        // Ensure directory exists
        if (!FileExist(TRADERX_DEBUG_TEST_CASES_DIR))
        {
            MakeDirectory(TRADERX_DEBUG_TEST_CASES_DIR);
        }
        
        testCase.SaveToFile(filePath);
        GetTraderXLogger().LogDebug(string.Format("[DEBUG MODE] Test case saved to: %1", filePath));
    }
    
    /**
     * Save all captured test cases as JSON files for execution at restart
     */
    void SaveAllTestCasesToJSON()
    {
        if (m_capturedTestCases.Count() == 0)
        {
            GetTraderXLogger().LogWarning("[DEBUG MODE] No test cases captured to save");
            return;
        }
        
        // Ensure directory exists
        if (!FileExist(TRADERX_DEBUG_TEST_CASES_DIR))
        {
            MakeDirectory(TRADERX_DEBUG_TEST_CASES_DIR);
        }
        
        int savedCount = 0;
        foreach (TraderXTransactionTestCase testCase : m_capturedTestCases)
        {
            string safeTestCaseId = testCase.testCaseId;
            string fileName = string.Format("TraderX_TestCase_%1.json", safeTestCaseId.Substring(0, 8));
            
            string filePath = TRADERX_DEBUG_TEST_CASES_DIR + fileName;
            
            // Save using TraderXJsonLoader
            testCase.SaveToFile(filePath);
            savedCount++;
        }
        
        GetTraderXLogger().LogDebug(string.Format("[DEBUG MODE] Saved %1 test cases as JSON files to: %2", savedCount, TRADERX_DEBUG_TEST_CASES_DIR));
        GetTraderXLogger().LogDebug("[DEBUG MODE] Test cases will be automatically loaded and executed when the test suite runs");
    }
    
    void ClearCapturedTestCases()
    {
        m_capturedTestCases.Clear();
        GetTraderXLogger().LogDebug("[DEBUG MODE] Cleared all captured test cases");
    }
    
    int GetCapturedTestCaseCount()
    {
        return m_capturedTestCases.Count();
    }
    
    array<ref TraderXTransactionTestCase> GetCapturedTestCases()
    {
        return m_capturedTestCases;
    }
    
    void PrintDebugStatus()
    {
        GetTraderXLogger().LogInfo("=== TRADERX DEBUG MODE STATUS ===");
        if(m_debugModeEnabled)
        {
            GetTraderXLogger().LogInfo("Debug Mode: ENABLED");
        }
        else
        {
            GetTraderXLogger().LogInfo("Debug Mode: DISABLED");
        }
        GetTraderXLogger().LogInfo(string.Format("Captured Test Cases: %1", m_capturedTestCases.Count()));
        
        if(m_currentTestCase)
        {
            GetTraderXLogger().LogInfo(string.Format("Current Capture: %1", "IN PROGRESS"));
        }
        else
        {
            GetTraderXLogger().LogInfo("Current Capture: NONE");
        }
        
        if (m_capturedTestCases.Count() > 0)
        {
            GetTraderXLogger().LogInfo("Recent Test Cases:");
            int maxShow = Math.Min(5, m_capturedTestCases.Count());
            for (int i = m_capturedTestCases.Count() - maxShow; i < m_capturedTestCases.Count(); i++)
            {
                TraderXTransactionTestCase testCase = m_capturedTestCases[i];
                GetTraderXLogger().LogInfo(string.Format("  - %1: %2 (Success Expected: %3)", testCase.testCaseId.Substring(0, 8), testCase.description, testCase.isSuccessExpected));
            }
        }
        GetTraderXLogger().LogInfo("=== END DEBUG STATUS ===");
    }
}
