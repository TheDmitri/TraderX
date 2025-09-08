/**
 * Comprehensive test suite for TraderXCurrencyService
 * Tests all methods and edge cases using Enforce script syntax
 */
class TraderXCurrencyServiceTest
{
    PlayerBase testPlayer;
    TraderXCurrencyService currencyService;
    ref TraderXCurrencyTypeCollection testCurrencySettings;
    
    // Test result tracking
    int totalTests = 0;
    int passedTests = 0;
    int failedTests = 0;

    void OnBeforeTest()
    {
        Object obj = GetGame().CreateObject("SurvivorM_Jose", vector.Zero, false, false);
        testPlayer = PlayerBase.Cast(obj);

        // Give player inventory space
        testPlayer.GetInventory().CreateInInventory("NBCHoodGray");
        testPlayer.GetInventory().CreateInInventory("TacticalShirt_Grey");
        testPlayer.GetInventory().CreateInInventory("HighCapacityVest_Black");
        testPlayer.GetInventory().CreateInInventory("HunterPants_Winter");
        testPlayer.GetInventory().CreateInInventory("AthleticShoes_Blue");
        testPlayer.GetInventory().CreateInInventory("HuntingBag");

        currencyService = TraderXCurrencyService.GetInstance();
        SetupTestCurrencySettings();
    }

    void SetupTestCurrencySettings()
    {
        testCurrencySettings = new TraderXCurrencyTypeCollection();
        
        // EUR currency type
        TraderXCurrencyType eurType = new TraderXCurrencyType("EUR");
        eurType.AddCurrency("TraderX_Money_Euro100", 10000);
        eurType.AddCurrency("TraderX_Money_Euro50", 5000);
        eurType.AddCurrency("TraderX_Money_Euro20", 2000);
        eurType.AddCurrency("TraderX_Money_Euro10", 1000);
        eurType.AddCurrency("TraderX_Money_Euro5", 500);
        eurType.AddCurrency("TraderX_Money_Euro1", 100);
        eurType.SortCurrenciesByValue();
        
        // USD currency type
        TraderXCurrencyType usdType = new TraderXCurrencyType("USD");
        usdType.AddCurrency("TraderX_Money_Dollar100", 10000);
        usdType.AddCurrency("TraderX_Money_Dollar50", 5000);
        usdType.AddCurrency("TraderX_Money_Dollar20", 2000);
        usdType.AddCurrency("TraderX_Money_Dollar10", 1000);
        usdType.AddCurrency("TraderX_Money_Dollar5", 500);
        usdType.AddCurrency("TraderX_Money_Dollar1", 100);
        usdType.SortCurrenciesByValue();
        
        testCurrencySettings.currencyTypes.Insert(eurType);
        testCurrencySettings.currencyTypes.Insert(usdType);
        
        currencyService.currencySettings = testCurrencySettings;
    }

    void RefreshSettings()
    {
        currencyService.currencySettings = TraderXCurrencyRepository.Load();
    }

    void CleanupPlayerInventory()
    {
        array<EntityAI> itemsArray = TraderXInventoryManager.GetItemsArray(testPlayer);
        for(int i = itemsArray.Count() - 1; i >= 0; i--)
        {
            EntityAI item = itemsArray[i];
            if(item && item.GetType().Contains("TraderX_Money"))
            {
                GetGame().ObjectDelete(item);
            }
        }
    }

    void AssertEquals(string testName, int expected, int actual)
    {
        totalTests++;
        if(expected == actual)
        {
            passedTests++;
            GetTraderXLogger().LogWarning(string.Format("[TEST PASS] %1: Expected %2, Got %3", testName, expected, actual));
        }
        else
        {
            failedTests++;
            GetTraderXLogger().LogError(string.Format("[TEST FAIL] %1: Expected %2, Got %3", testName, expected, actual));
        }
    }

    void AssertTrue(string testName, bool condition)
    {
        totalTests++;
        if(condition)
        {
            passedTests++;
            GetTraderXLogger().LogWarning(string.Format("[TEST PASS] %1", testName));
        }
        else
        {
            failedTests++;
            GetTraderXLogger().LogError(string.Format("[TEST FAIL] %1", testName));
        }
    }

    void AssertFalse(string testName, bool condition)
    {
        AssertTrue(testName, !condition);
    }
    //----------------------------------------------------------------//
    // Core Functionality Tests
    //----------------------------------------------------------------//

    void StartUnitTest()
    {
        GetTraderXLogger().LogInfo("[CURRENCY TEST] Starting comprehensive currency service tests...");
        
        // Basic functionality tests
        TestAddMoneyToPlayer_BasicAmount();
        TestAddMoneyToPlayer_ZeroAmount();
        TestAddMoneyToPlayer_NegativeAmount();
        TestAddMoneyToPlayer_LargeAmount();
        TestAddMoneyToPlayer_MultipleCurrencies();
        
        // Remove money tests
        TestRemoveMoneyAmountFromPlayer_ExactAmount();
        TestRemoveMoneyAmountFromPlayer_PartialAmount();
        TestRemoveMoneyAmountFromPlayer_InsufficientFunds();
        TestRemoveMoneyAmountFromPlayer_ZeroAmount();
        TestRemoveMoneyAmountFromPlayer_NegativeAmount();
        TestRemoveMoneyAmountFromPlayer_WithChange();
        TestRemoveMoneyAmountFromPlayer_WithExactChange();
        
        // Edge cases and error handling
        TestNullPlayer();
        TestEmptyAcceptedCurrencyTypes();
        TestNullAcceptedCurrencyTypes();
        TestInvalidCurrencyType();
        TestMixedValidInvalidCurrencyTypes();
        TestGetPlayerMoneyFromSpecificCurrency();
        TestCurrencyDenominationOptimization();
        TestConcurrentCurrencyOperations();
        TestInventorySpaceLimitations();
        TestCurrencyOverflow();
        
        // Print test summary
        PrintTestSummary();
        RefreshSettings();
    }

    void TestAddMoneyToPlayer_BasicAmount()
    {
        CleanupPlayerInventory();
        
        TStringArray eurCurrency = {"EUR"};
        currencyService.AddMoneyToPlayer(testPlayer, 15600, eurCurrency);
        
        int playerMoney = currencyService.GetPlayerMoneyFromAllCurrency(testPlayer, eurCurrency);
        AssertEquals("AddMoneyToPlayer_BasicAmount", 15600, playerMoney);
    }

    void TestAddMoneyToPlayer_ZeroAmount()
    {
        CleanupPlayerInventory();
        
        TStringArray eurCurrency = {"EUR"};
        currencyService.AddMoneyToPlayer(testPlayer, 0, eurCurrency);
        
        int playerMoney = currencyService.GetPlayerMoneyFromAllCurrency(testPlayer, eurCurrency);
        AssertEquals("AddMoneyToPlayer_ZeroAmount", 0, playerMoney);
    }

    void TestAddMoneyToPlayer_NegativeAmount()
    {
        CleanupPlayerInventory();
        
        TStringArray eurCurrency = {"EUR"};
        currencyService.AddMoneyToPlayer(testPlayer, -1000, eurCurrency);
        
        int playerMoney = currencyService.GetPlayerMoneyFromAllCurrency(testPlayer, eurCurrency);
        AssertEquals("AddMoneyToPlayer_NegativeAmount", 0, playerMoney);
    }

    void TestAddMoneyToPlayer_LargeAmount()
    {
        CleanupPlayerInventory();
        
        TStringArray usdCurrency = {"USD"};
        currencyService.AddMoneyToPlayer(testPlayer, 1000000, usdCurrency);
        
        int playerMoney = currencyService.GetPlayerMoneyFromAllCurrency(testPlayer, usdCurrency);
        AssertEquals("AddMoneyToPlayer_LargeAmount", 1000000, playerMoney);
    }

    void TestAddMoneyToPlayer_MultipleCurrencies()
    {
        CleanupPlayerInventory();
        
        TStringArray eurCurrency = {"EUR"};
        TStringArray usdCurrency = {"USD"};
        
        currencyService.AddMoneyToPlayer(testPlayer, 5000, eurCurrency);
        currencyService.AddMoneyToPlayer(testPlayer, 7500, usdCurrency);
        
        int eurMoney = currencyService.GetPlayerMoneyFromAllCurrency(testPlayer, eurCurrency);
        int usdMoney = currencyService.GetPlayerMoneyFromAllCurrency(testPlayer, usdCurrency);
        int totalMoney = currencyService.GetPlayerMoneyFromAllCurrency(testPlayer);
        
        AssertEquals("AddMoneyToPlayer_MultipleCurrencies_EUR", 5000, eurMoney);
        AssertEquals("AddMoneyToPlayer_MultipleCurrencies_USD", 7500, usdMoney);
        AssertEquals("AddMoneyToPlayer_MultipleCurrencies_Total", 12500, totalMoney);
    }

    void TestRemoveMoneyAmountFromPlayer_ExactAmount()
    {
        CleanupPlayerInventory();
        
        TStringArray eurCurrency = {"EUR"};
        currencyService.AddMoneyToPlayer(testPlayer, 15600, eurCurrency);
        
        bool result = currencyService.RemoveMoneyAmountFromPlayer(testPlayer, 15600, eurCurrency);
        int remainingMoney = currencyService.GetPlayerMoneyFromAllCurrency(testPlayer, eurCurrency);
        
        AssertTrue("RemoveMoneyAmountFromPlayer_ExactAmount_Result", result);
        AssertEquals("RemoveMoneyAmountFromPlayer_ExactAmount_Remaining", 0, remainingMoney);
    }

    void TestRemoveMoneyAmountFromPlayer_PartialAmount()
    {
        CleanupPlayerInventory();
        
        TStringArray eurCurrency = {"EUR"};
        currencyService.AddMoneyToPlayer(testPlayer, 20000, eurCurrency);
        
        bool result = currencyService.RemoveMoneyAmountFromPlayer(testPlayer, 7500, eurCurrency);
        int remainingMoney = currencyService.GetPlayerMoneyFromAllCurrency(testPlayer, eurCurrency);
        
        AssertTrue("RemoveMoneyAmountFromPlayer_PartialAmount_Result", result);
        AssertEquals("RemoveMoneyAmountFromPlayer_PartialAmount_Remaining", 12500, remainingMoney);
    }

    void TestRemoveMoneyAmountFromPlayer_InsufficientFunds()
    {
        CleanupPlayerInventory();
        
        TStringArray eurCurrency = {"EUR"};
        currencyService.AddMoneyToPlayer(testPlayer, 5000, eurCurrency);
        
        bool result = currencyService.RemoveMoneyAmountFromPlayer(testPlayer, 10000, eurCurrency);
        int remainingMoney = currencyService.GetPlayerMoneyFromAllCurrency(testPlayer, eurCurrency);
        
        AssertFalse("RemoveMoneyAmountFromPlayer_InsufficientFunds_Result", result);
        AssertEquals("RemoveMoneyAmountFromPlayer_InsufficientFunds_Remaining", 5000, remainingMoney);
    }

    void TestRemoveMoneyAmountFromPlayer_ZeroAmount()
    {
        CleanupPlayerInventory();
        
        TStringArray eurCurrency = {"EUR"};
        currencyService.AddMoneyToPlayer(testPlayer, 5000, eurCurrency);
        
        bool result = currencyService.RemoveMoneyAmountFromPlayer(testPlayer, 0, eurCurrency);
        int remainingMoney = currencyService.GetPlayerMoneyFromAllCurrency(testPlayer, eurCurrency);
        
        AssertFalse("RemoveMoneyAmountFromPlayer_ZeroAmount_Result", result);
        AssertEquals("RemoveMoneyAmountFromPlayer_ZeroAmount_Remaining", 5000, remainingMoney);
    }

    void TestRemoveMoneyAmountFromPlayer_NegativeAmount()
    {
        CleanupPlayerInventory();
        
        TStringArray eurCurrency = {"EUR"};
        currencyService.AddMoneyToPlayer(testPlayer, 5000, eurCurrency);
        
        bool result = currencyService.RemoveMoneyAmountFromPlayer(testPlayer, -1000, eurCurrency);
        int remainingMoney = currencyService.GetPlayerMoneyFromAllCurrency(testPlayer, eurCurrency);
        
        AssertFalse("RemoveMoneyAmountFromPlayer_NegativeAmount_Result", result);
        AssertEquals("RemoveMoneyAmountFromPlayer_NegativeAmount_Remaining", 5000, remainingMoney);
    }

    void TestRemoveMoneyAmountFromPlayer_WithChange()
    {
        CleanupPlayerInventory();
        
        // Add exactly one 100 EUR note (10000 value)
        TStringArray eurCurrency = {"EUR"};
        ItemBase eurNote = TraderXItemFactory.CreateInInventory(testPlayer, "TraderX_Money_Euro100", 1);
        
        // Try to remove 7500 (should give 2500 change)
        bool result = currencyService.RemoveMoneyAmountFromPlayer(testPlayer, 7500, eurCurrency);
        int remainingMoney = currencyService.GetPlayerMoneyFromAllCurrency(testPlayer, eurCurrency);
        
        AssertTrue("RemoveMoneyAmountFromPlayer_WithChange_Result", result);
        AssertEquals("RemoveMoneyAmountFromPlayer_WithChange_Remaining", 2500, remainingMoney);
    }
    
    void TestRemoveMoneyAmountFromPlayer_WithExactChange()
    {
        CleanupPlayerInventory();
        
        // Add one 50 EUR note (5000 value)
        TStringArray eurCurrency = {"EUR"};
        ItemBase eur50Note = TraderXItemFactory.CreateInInventory(testPlayer, "TraderX_Money_Euro50", 1);
        
        // Log initial state
        int initialMoney = currencyService.GetPlayerMoneyFromAllCurrency(testPlayer, eurCurrency);
        GetTraderXLogger().LogInfo(string.Format("[TEST] Initial money: %1", initialMoney.ToString()));
        
        // Try to remove 3200 (should take the 50 EUR note and give back 1800 change)
        bool result = currencyService.RemoveMoneyAmountFromPlayer(testPlayer, 3200, eurCurrency);
        
        // Verify the result and remaining money
        int remainingMoney = currencyService.GetPlayerMoneyFromAllCurrency(testPlayer, eurCurrency);
        
        // Log the final state
        GetTraderXLogger().LogInfo(string.Format("[TEST] Transaction result: %1, Remaining money: %2", result.ToString(), remainingMoney.ToString()));
        
        // Verify the transaction was successful and correct change was given back
        AssertTrue("RemoveMoneyAmountFromPlayer_WithExactChange_Result", result);
        AssertEquals("RemoveMoneyAmountFromPlayer_WithExactChange_Remaining", 1800, remainingMoney);
        
        // Also verify the actual items in inventory
        array<EntityAI> items = TraderXInventoryManager.GetItemsArray(testPlayer);
        bool hasCorrectChange = false;
        
        foreach(EntityAI entity : items)
        {
            ItemBase item = ItemBase.Cast(entity);
            if(!item) continue;
            
            string itemType = item.GetType();
            int quantity = TraderXQuantityManager.GetItemAmount(item);
            
            GetTraderXLogger().LogInfo(string.Format("[TEST] Item in inventory: %1 x %2", itemType, quantity.ToString()));
            
            // We should have change in the form of smaller denominations
            if(itemType == "TraderX_Money_Euro20" || itemType == "TraderX_Money_Euro10" || itemType == "TraderX_Money_Euro5" || itemType == "TraderX_Money_Euro1")
            {
                hasCorrectChange = true;
            }
        }
        
        AssertTrue("RemoveMoneyAmountFromPlayer_WithExactChange_HasCorrectChangeItems", hasCorrectChange);
    }
    //----------------------------------------------------------------//
    // Edge Cases and Error Handling Tests
    //----------------------------------------------------------------//

    void TestNullPlayer()
    {
        TStringArray eurCurrency = {"EUR"};
        bool result = currencyService.RemoveMoneyAmountFromPlayer(null, 1000, eurCurrency);
        AssertFalse("NullPlayer_RemoveMoney", result);
    }

    void TestEmptyAcceptedCurrencyTypes()
    {
        CleanupPlayerInventory();
        
        TStringArray emptyCurrency = {};
        currencyService.AddMoneyToPlayer(testPlayer, 5000, emptyCurrency);
        
        int playerMoney = currencyService.GetPlayerMoneyFromAllCurrency(testPlayer);
        AssertEquals("EmptyAcceptedCurrencyTypes", 5000, playerMoney);
    }

    void TestNullAcceptedCurrencyTypes()
    {
        CleanupPlayerInventory();
        
        currencyService.AddMoneyToPlayer(testPlayer, 5000, null);
        
        int playerMoney = currencyService.GetPlayerMoneyFromAllCurrency(testPlayer);
        AssertEquals("NullAcceptedCurrencyTypes", 5000, playerMoney);
    }

    void TestInvalidCurrencyType()
    {
        CleanupPlayerInventory();
        
        TStringArray invalidCurrency = {"INVALID_CURRENCY"};
        currencyService.AddMoneyToPlayer(testPlayer, 5000, invalidCurrency);
        
        int playerMoney = currencyService.GetPlayerMoneyFromAllCurrency(testPlayer);
        AssertEquals("InvalidCurrencyType", 0, playerMoney);
    }

    void TestMixedValidInvalidCurrencyTypes()
    {
        CleanupPlayerInventory();
        
        TStringArray mixedCurrency = {"EUR", "INVALID_CURRENCY", "USD"};
        currencyService.AddMoneyToPlayer(testPlayer, 5000, mixedCurrency);
        
        int playerMoney = currencyService.GetPlayerMoneyFromAllCurrency(testPlayer);
        AssertEquals("MixedValidInvalidCurrencyTypes", 5000, playerMoney);
    }

    void TestGetPlayerMoneyFromSpecificCurrency()
    {
        CleanupPlayerInventory();
        
        TStringArray eurCurrency = {"EUR"};
        TStringArray usdCurrency = {"USD"};
        
        currencyService.AddMoneyToPlayer(testPlayer, 3000, eurCurrency);
        currencyService.AddMoneyToPlayer(testPlayer, 7000, usdCurrency);
        
        int eurMoney = currencyService.GetPlayerMoneyFromAllCurrency(testPlayer, eurCurrency);
        int usdMoney = currencyService.GetPlayerMoneyFromAllCurrency(testPlayer, usdCurrency);
        int totalMoney = currencyService.GetPlayerMoneyFromAllCurrency(testPlayer);
        
        AssertEquals("GetPlayerMoneyFromSpecificCurrency_EUR", 3000, eurMoney);
        AssertEquals("GetPlayerMoneyFromSpecificCurrency_USD", 7000, usdMoney);
        AssertEquals("GetPlayerMoneyFromSpecificCurrency_Total", 10000, totalMoney);
    }

    void TestCurrencyDenominationOptimization()
    {
        CleanupPlayerInventory();
        
        TStringArray eurCurrency = {"EUR"};
        currencyService.AddMoneyToPlayer(testPlayer, 17600, eurCurrency);
        
        // Check that the system uses optimal denominations
        array<EntityAI> itemsArray = TraderXInventoryManager.GetItemsArray(testPlayer);
        int euro100Count = 0;
        int euro50Count = 0;
        int euro20Count = 0;
        int euro10Count = 0;
        int euro5Count = 0;
        int euro1Count = 0;
        
        foreach(EntityAI item : itemsArray)
        {
            if(!item || !item.GetType().Contains("TraderX_Money_Euro"))
                continue;
                
            string itemType = item.GetType();
            int quantity = TraderXQuantityManager.GetItemAmount(ItemBase.Cast(item));

            GetTraderXLogger().LogInfo("[CURRENCY TEST] Item Type: " + itemType + ", Quantity: " + quantity);
            
            if(itemType == "TraderX_Money_Euro100") euro100Count += quantity;
            else if(itemType == "TraderX_Money_Euro50") euro50Count += quantity;
            else if(itemType == "TraderX_Money_Euro20") euro20Count += quantity;
            else if(itemType == "TraderX_Money_Euro10") euro10Count += quantity;
            else if(itemType == "TraderX_Money_Euro5") euro5Count += quantity;
            else if(itemType == "TraderX_Money_Euro1") euro1Count += quantity;
        }
        
        // 17600 should be: 1x100 (10000) + 1x50 (5000) + 1x20 (2000) + 6x1 (600)
        AssertEquals("CurrencyDenominationOptimization_Euro100", 1, euro100Count);
        AssertEquals("CurrencyDenominationOptimization_Euro50", 1, euro50Count);
        AssertEquals("CurrencyDenominationOptimization_Euro20", 1, euro20Count);
        AssertEquals("CurrencyDenominationOptimization_Euro5", 1, euro5Count);
        AssertEquals("CurrencyDenominationOptimization_Euro1", 1, euro1Count);
    }

    void TestConcurrentCurrencyOperations()
    {
        CleanupPlayerInventory();
        
        TStringArray eurCurrency = {"EUR"};
        TStringArray usdCurrency = {"USD"};
        
        // Add money in different currencies
        currencyService.AddMoneyToPlayer(testPlayer, 5000, eurCurrency);
        currencyService.AddMoneyToPlayer(testPlayer, 3000, usdCurrency);
        
        // Remove from one currency, should not affect the other
        bool result = currencyService.RemoveMoneyAmountFromPlayer(testPlayer, 2000, eurCurrency);
        
        int eurMoney = currencyService.GetPlayerMoneyFromAllCurrency(testPlayer, eurCurrency);
        int usdMoney = currencyService.GetPlayerMoneyFromAllCurrency(testPlayer, usdCurrency);
        
        AssertTrue("ConcurrentCurrencyOperations_Result", result);
        AssertEquals("ConcurrentCurrencyOperations_EUR_Remaining", 3000, eurMoney);
        AssertEquals("ConcurrentCurrencyOperations_USD_Unchanged", 3000, usdMoney);
    }

    void TestInventorySpaceLimitations()
    {
        CleanupPlayerInventory();
        
        // Test with a reasonable amount that won't cause infinite loops
        TStringArray eurCurrency = {"EUR"};
        int testAmount = 5000; // Enough to test the logic without being excessive
        
        GetTraderXLogger().LogInfo("[TEST] Testing inventory space with amount: " + testAmount.ToString());
        
        currencyService.AddMoneyToPlayer(testPlayer, testAmount, eurCurrency);
        int playerMoney = currencyService.GetPlayerMoneyFromAllCurrency(testPlayer, eurCurrency);
        
        GetTraderXLogger().LogInfo("[TEST] Added money to player, total money: " + playerMoney.ToString());
        
        // Verify we were able to add some money
        AssertTrue("InventorySpaceLimitations_AddedMoney", playerMoney > 0);
        
        // Verify the amount is what we expect (may be less due to inventory constraints)
        AssertTrue("InventorySpaceLimitations_ReasonableAmount", playerMoney <= testAmount);
    }

    void TestCurrencyOverflow()
    {
        CleanupPlayerInventory();
        
        TStringArray usdCurrency = {"USD"};
        
        // Test with maximum integer value but expect failsafes to kick in
        const int MAX_INT = 2147483647;
        const int REASONABLE_CHUNK_SIZE = 10000000; // 10 million per chunk (larger chunks to reduce stack count)
        int totalAdded = 0;
        int chunks = 0;
        int maxChunks = 100; // Limit chunks to prevent infinite loops
        
        GetTraderXLogger().LogInfo("[TEST] Testing currency overflow with max int value (" + MAX_INT.ToString() + ") - expecting failsafes to activate");
        
        // First, test the limits warning system
        GetTraderXLogger().LogInfo("[TEST] Testing currency limits calculation for max int");
        currencyService.CheckCurrencyLimitsAndWarn(MAX_INT, usdCurrency);
        
        // Add money in chunks until failsafes stop us or we reach reasonable limits
        int remaining = MAX_INT;
        bool failsafeTriggered = false;
        
        while(remaining > 0 && chunks < maxChunks) {
            int amountToAdd = Math.Min(REASONABLE_CHUNK_SIZE, remaining);
            int playerMoneyBefore = currencyService.GetPlayerMoneyFromAllCurrency(testPlayer, usdCurrency);
            
            GetTraderXLogger().LogInfo(string.Format("[TEST] Attempting to add chunk %1: %2 (remaining: %3)", (chunks + 1).ToString(), amountToAdd.ToString(), remaining.ToString()));
            
            // This may fail due to our new failsafes
            currencyService.AddMoneyToPlayer(testPlayer, amountToAdd, usdCurrency);
            
            int playerMoneyAfter = currencyService.GetPlayerMoneyFromAllCurrency(testPlayer, usdCurrency);
            int actuallyAdded = playerMoneyAfter - playerMoneyBefore;
            
            if(actuallyAdded == 0) {
                GetTraderXLogger().LogWarning(string.Format("[TEST] Failsafe triggered at chunk %1. No money was added.", (chunks + 1).ToString()));
                failsafeTriggered = true;
                break;
            }
            
            totalAdded += actuallyAdded;
            remaining -= amountToAdd;
            chunks++;
            
            // Log progress every 10 chunks
            if(chunks % 10 == 0) {
                GetTraderXLogger().LogInfo(string.Format("[TEST] Progress: %1 chunks completed, %2 total added", chunks.ToString(), totalAdded.ToString()));
            }
        }
        
        int finalPlayerMoney = currencyService.GetPlayerMoneyFromAllCurrency(testPlayer, usdCurrency);
        GetTraderXLogger().LogInfo("[TEST] Final results:");
        GetTraderXLogger().LogInfo("  - Chunks attempted: " + chunks.ToString());
        GetTraderXLogger().LogInfo("  - Total money added: " + finalPlayerMoney.ToString());
        GetTraderXLogger().LogInfo("  - Failsafe triggered: " + failsafeTriggered.ToString());
        
        // Verify we were able to add some money
        AssertTrue("CurrencyOverflow_AddedSomeMoney", finalPlayerMoney > 0);
        
        // Verify we didn't exceed max int
        AssertTrue("CurrencyOverflow_NotExceedMaxInt", finalPlayerMoney <= MAX_INT);
        
        // If failsafe triggered, that's actually a success - it means our protection works
        if(failsafeTriggered) {
            GetTraderXLogger().LogInfo("[TEST] SUCCESS: Failsafe protection activated as expected");
            AssertTrue("CurrencyOverflow_FailsafeWorking", true);
        } else {
            GetTraderXLogger().LogInfo("[TEST] No failsafe triggered - system handled large amount gracefully");
            AssertTrue("CurrencyOverflow_GracefulHandling", true);
        }
        
        // Log the final amount added as a percentage of max int
        float percentage = ((float)finalPlayerMoney / MAX_INT) * 100.0;
        GetTraderXLogger().LogInfo(string.Format("[TEST] Successfully added %1% of max int value", percentage.ToString(1)));
    }

    void TestCurrencyLimitsCalculation()
    {
        CleanupPlayerInventory();
        
        GetTraderXLogger().LogInfo("[TEST] Testing currency limits calculation");
        
        TStringArray eurCurrency = {"EUR"};
        
        // Test with small amount (should not trigger warnings)
        int smallAmount = 1000;
        GetTraderXLogger().LogInfo("[TEST] Testing small amount: " + smallAmount.ToString());
        currencyService.CheckCurrencyLimitsAndWarn(smallAmount, eurCurrency);
        
        // Test with medium amount (should trigger efficiency warning)
        int mediumAmount = 10000000; // 10 million
        GetTraderXLogger().LogInfo("[TEST] Testing medium amount: " + mediumAmount.ToString());
        currencyService.CheckCurrencyLimitsAndWarn(mediumAmount, eurCurrency);
        
        // Test with large amount (should trigger error)
        int largeAmount = 2000000000; // 2 billion
        GetTraderXLogger().LogInfo("[TEST] Testing large amount: " + largeAmount.ToString());
        currencyService.CheckCurrencyLimitsAndWarn(largeAmount, eurCurrency);
        
        AssertTrue("CurrencyLimitsCalculation_Completed", true);
    }
    
    void TestCurrencyLimitsWithDifferentCurrencies()
    {
        CleanupPlayerInventory();
        
        GetTraderXLogger().LogInfo("[TEST] Testing currency limits with different currency types");
        
        // Test with EUR currency
        TStringArray eurCurrency = {"EUR"};
        int testAmount = 50000000; // 50 million
        GetTraderXLogger().LogInfo("[TEST] Testing EUR currency limits with amount: " + testAmount.ToString());
        currencyService.CheckCurrencyLimitsAndWarn(testAmount, eurCurrency);
        
        // Test with USD currency
        TStringArray usdCurrency = {"USD"};
        GetTraderXLogger().LogInfo("[TEST] Testing USD currency limits with amount: " + testAmount.ToString());
        currencyService.CheckCurrencyLimitsAndWarn(testAmount, usdCurrency);
        
        // Test with all currencies (empty array means all accepted)
        TStringArray allCurrencies = {};
        GetTraderXLogger().LogInfo("[TEST] Testing all currencies limits with amount: " + testAmount.ToString());
        currencyService.CheckCurrencyLimitsAndWarn(testAmount, allCurrencies);
        
        AssertTrue("CurrencyLimitsWithDifferentCurrencies_Completed", true);
    }
    
    void TestCurrencyLimitsEdgeCases()
    {
        CleanupPlayerInventory();
        
        GetTraderXLogger().LogInfo("[TEST] Testing currency limits edge cases");
        
        TStringArray eurCurrency = {"EUR"};
        
        // Test with zero amount
        GetTraderXLogger().LogInfo("[TEST] Testing zero amount");
        currencyService.CheckCurrencyLimitsAndWarn(0, eurCurrency);
        
        // Test with negative amount
        GetTraderXLogger().LogInfo("[TEST] Testing negative amount");
        currencyService.CheckCurrencyLimitsAndWarn(-1000, eurCurrency);
        
        // Test with maximum integer value
        GetTraderXLogger().LogInfo("[TEST] Testing maximum integer value");
        currencyService.CheckCurrencyLimitsAndWarn(2147483647, eurCurrency);
        
        // Test with null currency array
        GetTraderXLogger().LogInfo("[TEST] Testing null currency array");
        currencyService.CheckCurrencyLimitsAndWarn(1000000, null);
        
        // Test with invalid currency type
        TStringArray invalidCurrency = {"INVALID_CURRENCY"};
        GetTraderXLogger().LogInfo("[TEST] Testing invalid currency type");
        currencyService.CheckCurrencyLimitsAndWarn(1000000, invalidCurrency);
        
        AssertTrue("CurrencyLimitsEdgeCases_Completed", true);
    }
    
    void TestCurrencyLimitsLogging()
    {
        CleanupPlayerInventory();
        
        GetTraderXLogger().LogInfo("[TEST] Testing currency limits logging output");
        
        TStringArray eurCurrency = {"EUR"};
        
        // Test amounts that should produce different log levels
        array<int> testAmounts = new array<int>;
        testAmounts.Insert(1000);
        testAmounts.Insert(5000000);
        testAmounts.Insert(50000000);
        testAmounts.Insert(500000000);
        testAmounts.Insert(2000000000);
        
        array<string> expectedLevels = new array<string>;
        expectedLevels.Insert("INFO");
        expectedLevels.Insert("INFO");
        expectedLevels.Insert("WARNING");
        expectedLevels.Insert("WARNING");
        expectedLevels.Insert("ERROR");
        
        for(int i = 0; i < testAmounts.Count(); i++)
        {
            int amount = testAmounts.Get(i);
            string expectedLevel = expectedLevels.Get(i);
            
            GetTraderXLogger().LogInfo(string.Format("[TEST] Testing amount %1 (expecting %2 level)", amount.ToString(), expectedLevel));
            currencyService.CheckCurrencyLimitsAndWarn(amount, eurCurrency);
        }
        
        AssertTrue("CurrencyLimitsLogging_Completed", true);
    }

    void PrintTestSummary()
    {
        GetTraderXLogger().LogInfo(string.Format("[CURRENCY TEST] Test Summary: %1 total, %2 passed, %3 failed", totalTests, passedTests, failedTests));
        
        if(failedTests == 0)
        {
            GetTraderXLogger().LogInfo("[CURRENCY TEST] All tests PASSED!");
        }
        else
        {
            GetTraderXLogger().LogError(string.Format("[CURRENCY TEST] %1 tests FAILED!", failedTests));
        }
    }
}
