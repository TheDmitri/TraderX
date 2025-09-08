/**
 * TraderXTestCaseLoader - Loads and executes JSON test cases in the test suite
 * Allows running captured real-world scenarios as automated tests
 */
class TraderXTestCaseLoader
{
    static ref TraderXTestCaseLoader m_instance;
    
    void TraderXTestCaseLoader()
    {
    }
    
    static TraderXTestCaseLoader GetInstance()
    {
        if (!m_instance)
            m_instance = new TraderXTestCaseLoader();
        return m_instance;
    }
    
    /**
     * Load and execute all JSON test cases from the debug directory
     */
    void RunAllJSONTestCases(TraderXTransactionsTest testRunner)
    {
        GetTraderXLogger().LogInfo("[JSON TEST] Starting JSON test case execution...");
        
        if (!FileExist(TRADERX_DEBUG_TEST_CASES_DIR))
        {
            GetTraderXLogger().LogWarning("[JSON TEST] No test case directory found: " + TRADERX_DEBUG_TEST_CASES_DIR);
            return;
        }
        
        // Find all JSON files in the directory
        array<string> jsonFiles = FindJSONFiles(TRADERX_DEBUG_TEST_CASES_DIR);
        
        GetTraderXLogger().LogInfo(string.Format("[JSON TEST] Found %1 JSON test case files", jsonFiles.Count()));
        
        int passedTests = 0;
        int failedTests = 0;
        
        foreach (string jsonFile : jsonFiles)
        {
            GetTraderXLogger().LogInfo(string.Format("[JSON TEST] Executing test case: %1", jsonFile));
            
            if (ExecuteJSONTestCase(jsonFile, testRunner))
            {
                passedTests++;
            }
            else
            {
                failedTests++;
            }
        }
        
        GetTraderXLogger().LogInfo(string.Format("[JSON TEST] JSON test execution complete: %1 passed, %2 failed", passedTests, failedTests));
    }
    
    /**
     * Execute a specific JSON test case file
     */
    bool ExecuteJSONTestCase(string jsonFilePath, TraderXTransactionsTest testRunner)
    {
        // Load test case using TraderXJsonLoader
        TraderXTransactionTestCase testCase = TraderXTransactionTestCase.LoadFromFile(jsonFilePath);
        if (!testCase)
        {
            GetTraderXLogger().LogError("[JSON TEST] Failed to load test case from: " + jsonFilePath);
            return false;
        }

        GetTraderXLogger().LogInfo(string.Format("[JSON TEST] Executing test case: %1 - %2", testCase.testCaseId, testCase.description));
        
        // Restore product configurations before executing test
        RestoreProductConfigurations(testCase);
        
        // Execute the test case
        return ExecuteTestCase(testCase, testRunner);
    }
    
    /**
     * Restore product configurations from test case to ensure exact scenario reproduction
     */
    private void RestoreProductConfigurations(TraderXTransactionTestCase testCase)
    {
        if (!testCase.productConfigurations || testCase.productConfigurations.Count() == 0)
        {
            GetTraderXLogger().LogWarning("[JSON TEST] No product configurations to restore for test case: " + testCase.testCaseId);
            return;
        }
        
        GetTraderXLogger().LogInfo(string.Format("[JSON TEST] Restoring %1 product configurations for test case: %2", 
            testCase.productConfigurations.Count(), testCase.testCaseId));
        
        foreach (TraderXProduct savedProduct : testCase.productConfigurations)
        {
            // Get current product from repository
            TraderXProduct currentProduct = TraderXProductRepository.GetItemById(savedProduct.productId);
            if (currentProduct)
            {
                // Restore critical pricing and stock configuration
                currentProduct.buyPrice = savedProduct.buyPrice;
                currentProduct.sellPrice = savedProduct.sellPrice;
                currentProduct.coefficient = savedProduct.coefficient;
                currentProduct.maxStock = savedProduct.maxStock;
                currentProduct.tradeQuantity = savedProduct.tradeQuantity;
                currentProduct.stockSettings = savedProduct.stockSettings;
                
                GetTraderXLogger().LogInfo(string.Format("[JSON TEST] Restored config for %1: buyPrice=%2, coefficient=%3, maxStock=%4", 
                    savedProduct.productId, savedProduct.buyPrice, savedProduct.coefficient, savedProduct.maxStock));
            }
            else
            {
                GetTraderXLogger().LogWarning("[JSON TEST] Could not find product to restore: " + savedProduct.productId);
            }
        }
    }
    
    private array<string> FindJSONFiles(string directory)
    {
        array<string> jsonFiles = new array<string>();
        
        if (!FileExist(directory))
        {
            GetTraderXLogger().LogWarning("[JSON TEST] Directory does not exist: " + directory);
            return jsonFiles;
        }
        
        string filename;
        FileAttr attr;
        
        FindFileHandle findHandle = FindFile(directory + "*.json", filename, attr, FindFileFlags.ALL);
        
        if (findHandle == 0)
        {
            GetTraderXLogger().LogDebug("[JSON TEST] No JSON files found in directory: " + directory);
            CloseFindFile(findHandle);
            return jsonFiles;
        }
        
        // Process first file
        if (filename != "")
        {
            jsonFiles.Insert(directory + filename);
        }
        
        // Process remaining files
        while (FindNextFile(findHandle, filename, attr))
        {
            if (filename != "")
            {
                jsonFiles.Insert(directory + filename);
            }
        }
        
        CloseFindFile(findHandle);
        return jsonFiles;
    }
    
    private string LoadJSONFile(string filePath)
    {
        FileHandle file = OpenFile(filePath, FileMode.READ);
        if (!file)
        {
            return "";
        }
        
        string content = "";
        string line = "";
        
        while (FGets(file, line) >= 0)
        {
            content += line + "\n";
        }
        
        CloseFile(file);
        return content;
    }
    
    private bool ExecuteTestCase(TraderXTransactionTestCase testCase, TraderXTransactionsTest testRunner)
    {
        GetTraderXLogger().LogInfo(string.Format("[JSON TEST] Executing: %1 - %2", testCase.testCaseId, testCase.description));
        
        // Clear inventory and reset player state
        testRunner.ClearPlayerCargo();
        
        // Set up pre-transaction player state
        if (testCase.preTransactionState)
        {
            SetupPlayerState(testRunner.testPlayer, testCase.preTransactionState, testCase.traderId);
        }
        
        // Set up pre-transaction stock state
        if (testCase.preTransactionStock)
        {
            SetupStockState(testCase.preTransactionStock);
        }
        
        // Execute the transaction
        TraderXTransactionResultCollection actualResult = ExecuteTransaction(testCase.transactionCollection, testRunner.testPlayer);
        
        // Validate results
        bool testPassed = ValidateTestResult(testCase, actualResult, testRunner.testPlayer);
        
        if (testPassed)
        {
            GetTraderXLogger().LogInfo(string.Format("[JSON TEST] PASSED: %1", testCase.testCaseId));
        }
        else
        {
            GetTraderXLogger().LogError(string.Format("[JSON TEST] FAILED: %1", testCase.testCaseId));
        }
        
        return testPassed;
    }
    
    private void SetupPlayerState(PlayerBase player, TraderXPlayerState playerState, int traderId)
    {
        // Set up money
        TraderXNpc npc = GetTraderXModule().GetSettings().GetNpcById(traderId);
        if (npc && playerState.currencyTypes)
        {
            TraderXCurrencyService.GetInstance().AddMoneyToPlayer(player, playerState.totalMoney);
        }
        
        // Set up inventory items (excluding equipment and currency)
        if (playerState.inventoryItems)
        {
            foreach (TraderXTestInventoryItem item : playerState.inventoryItems)
            {
                if (!IsCurrencyItem(item.className, playerState.currencyTypes))
                {
                    EntityAI createdItem = player.GetInventory().CreateInInventory(item.className);
                    if (createdItem && item.quantity > 1)
                    {
                        ItemBase itemBase = ItemBase.Cast(createdItem);
                        if (itemBase)
                        {
                            itemBase.SetQuantity(item.quantity);
                        }
                    }
                }
            }
        }
    }
    
    private void SetupStockState(array<ref TraderXProductStock> stockArray)
    {
        foreach (TraderXProductStock stock : stockArray)
        {
            TraderXProductStock newStock = new TraderXProductStock(stock.productId, stock.stock);
            TraderXProductStockRepository.Save(newStock);
        }
    }
    
    private TraderXTransactionResultCollection ExecuteTransaction(TraderXTransactionCollection transactionCollection, PlayerBase player)
    {
        if (!transactionCollection)
        {
            return null;
        }
        
        TraderXTransactionCoordinator coordinator = TraderXTransactionCoordinator.GetInstance();
        array<ref TraderXTransactionResult> results = coordinator.ProcessTransactionBatch(transactionCollection, player);
        return TraderXTransactionResultCollection.Create(player.GetIdentity().GetPlainId(), results);
    }
    
    private bool ValidateTestResult(TraderXTransactionTestCase testCase, TraderXTransactionResultCollection actualResult, PlayerBase player)
    {
        if (!actualResult)
        {
            GetTraderXLogger().LogError(string.Format("[JSON TEST] No result returned for test case: %1", testCase.testCaseId));
            return false;
        }
        
        // Validate success/failure expectation
        bool actualSuccess = !actualResult.HasErrors();
        if (actualSuccess != testCase.isSuccessExpected)
        {
            GetTraderXLogger().LogError(string.Format("[JSON TEST] Expected success: %1, but got: %2 for test case: %3", testCase.isSuccessExpected, actualSuccess, testCase.testCaseId));
            return false;
        }
        
        // If success was expected, validate money changes
        if (testCase.isSuccessExpected && testCase.postTransactionState)
        {
            TraderXNpc npc = GetTraderXModule().GetSettings().GetNpcById(testCase.traderId);
            if (npc)
            {
                int actualMoney = TraderXCurrencyService.GetInstance().GetPlayerMoneyFromAllCurrency(player, npc.GetCurrenciesAccepted());
                if (actualMoney != testCase.postTransactionState.totalMoney)
                {
                    GetTraderXLogger().LogError(string.Format("[JSON TEST] Expected money: %1, but got: %2 for test case: %3", testCase.postTransactionState.totalMoney, actualMoney, testCase.testCaseId));
                    return false;
                }
            }
        }
        
        GetTraderXLogger().LogInfo(string.Format("[JSON TEST] Validation passed for test case: %1", testCase.testCaseId));
        return true;
    }
    
    private bool IsCurrencyItem(string className, array<string> currencyTypes)
    {
        if (!currencyTypes) return false;
        
        foreach (string currency : currencyTypes)
        {
            if (className.Contains(currency) || className.Contains("Money") || className.Contains("Cash"))
            {
                return true;
            }
        }
        return false;
    }
}
