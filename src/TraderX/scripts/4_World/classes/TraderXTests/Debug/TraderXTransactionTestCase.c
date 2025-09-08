/**
 * TraderXTransactionTestCase - Captures real transaction scenarios for debugging
 * This class records the complete state before/after a transaction for test reproduction
 */
class TraderXTransactionTestCase
{
    // Test case metadata
    string testCaseId;
    string timestamp;
    string playerSteamId;
    string description;
    bool isSuccessExpected;
    
    // Pre-transaction state
    ref TraderXPlayerState preTransactionState;
    ref array<ref TraderXProductStock> preTransactionStock;
    ref array<ref TraderXProduct> productConfigurations; // Capture product configs for exact reproduction
    
    // Transaction details
    ref TraderXTransactionCollection transactionCollection;
    int traderId;
    
    // Post-transaction state
    ref TraderXPlayerState postTransactionState;
    ref array<ref TraderXProductStock> postTransactionStock;
    ref TraderXTransactionResultCollection actualResult;
    
    // Expected result (for validation)
    ref TraderXTransactionResultCollection expectedResult;
    string expectedFailureReason;
    
    void TraderXTransactionTestCase()
    {
        testCaseId = TraderXTransactionId.Generate().GetValue();
        timestamp = CF_Date.Now().GetTimestamp().ToString();
        preTransactionState = new TraderXPlayerState();
        postTransactionState = new TraderXPlayerState();
        preTransactionStock = new array<ref TraderXProductStock>();
        postTransactionStock = new array<ref TraderXProductStock>();
        productConfigurations = new array<ref TraderXProduct>();
    }
    
    static TraderXTransactionTestCase Create(string _description, bool _successExpected = true)
    {
        TraderXTransactionTestCase testCase = new TraderXTransactionTestCase();
        testCase.description = _description;
        testCase.isSuccessExpected = _successExpected;
        return testCase;
    }
    
    void CapturePreTransactionState(PlayerBase player, TraderXTransactionCollection transactions, int npcId)
    {
        playerSteamId = player.GetIdentity().GetPlainId();
        transactionCollection = transactions;
        traderId = npcId;
        
        // Capture player state
        preTransactionState.CaptureState(player, traderId);
        
        // Capture stock state for all products in the transaction
        CaptureStockState(transactions, preTransactionStock);
        
        // Capture product configurations for exact reproduction
        CaptureProductConfigurations(transactions);
        
        GetTraderXLogger().LogDebug(string.Format("[DEBUG] Captured pre-transaction state for test case: %1", testCaseId));
    }
    
    void CapturePostTransactionState(PlayerBase player, TraderXTransactionResultCollection result)
    {
        actualResult = result;
        
        // Capture player state after transaction
        postTransactionState.CaptureState(player, traderId);
        
        // Capture stock state after transaction
        CaptureStockState(transactionCollection, postTransactionStock);
        
        GetTraderXLogger().LogDebug(string.Format("[DEBUG] Captured post-transaction state for test case: %1", testCaseId));
    }
    
    void SetExpectedResult(TraderXTransactionResultCollection expected, string failureReason = "")
    {
        expectedResult = expected;
        expectedFailureReason = failureReason;
    }
    
    private void CaptureStockState(TraderXTransactionCollection transactions, array<ref TraderXProductStock> stockArray)
    {
        stockArray.Clear();
        
        // Get unique product IDs from transactions
        array<string> productIds = new array<string>();
        for (int i = 0; i < transactions.GetAllTransactions().Count(); i++)
        {
            TraderXTransaction transaction = transactions.GetAllTransactions().Get(i);
            if (productIds.Find(transaction.GetProductId()) == -1)
            {
                productIds.Insert(transaction.GetProductId());
                
                // Also capture preset attachment stock if applicable
                TraderXPreset preset = transaction.GetPreset();
                if (preset && preset.attachments)
                {
                    for (int j = 0; j < preset.attachments.Count(); j++)
                    {
                        string attachmentId = preset.attachments.Get(j);
                        if (productIds.Find(attachmentId) == -1)
                        {
                            productIds.Insert(attachmentId);
                        }
                    }
                }
            }
        }
        
        // Capture current stock for all relevant products
        foreach (string productId : productIds)
        {
            TraderXProductStock stock = TraderXProductStockRepository.GetStockByProductId(productId);
            if (stock)
            {
                // Create a copy to avoid reference issues
                TraderXProductStock stockCopy = new TraderXProductStock(stock.productId, stock.GetStock());
                stockArray.Insert(stockCopy);
            }
        }
    }
    
    string ToFormattedString()
    {
        string output = "";
        output += "=== TRANSACTION TEST CASE ===\n";
        output += string.Format("ID: %1\n", testCaseId);
        output += string.Format("Timestamp: %1\n", timestamp);
        output += string.Format("Player: %1\n", playerSteamId);
        output += string.Format("Description: %1\n", description);
        output += string.Format("Success Expected: %1\n", isSuccessExpected);
        output += "\n--- PRE-TRANSACTION STATE ---\n";
        output += preTransactionState.ToFormattedString();
        output += "\n--- TRANSACTION DETAILS ---\n";
        output += transactionCollection.ToStringFormatted();
        output += "\n--- POST-TRANSACTION STATE ---\n";
        output += postTransactionState.ToFormattedString();
        output += "\n--- ACTUAL RESULT ---\n";
        if (actualResult)
            output += actualResult.ToStringFormatted();
        output += "\n--- EXPECTED RESULT ---\n";
        if (expectedResult)
            output += expectedResult.ToStringFormatted();
        else if (expectedFailureReason != "")
            output += string.Format("Expected Failure: %1\n", expectedFailureReason);
        output += "=== END TEST CASE ===\n";
        
        return output;
    }
    
    /**
     * Save test case to JSON file using TraderXJsonLoader
     */
    void SaveToFile(string filePath)
    {
        TraderXJsonLoader<TraderXTransactionTestCase>.SaveToFile(filePath, this);
    }
    
    private void CaptureProductConfigurations(TraderXTransactionCollection transactions)
    {
        productConfigurations.Clear();
        
        // Get unique product IDs from transactions
        array<string> productIds = new array<string>();
        for (int i = 0; i < transactions.GetAllTransactions().Count(); i++)
        {
            TraderXTransaction transaction = transactions.GetAllTransactions().Get(i);
            if (productIds.Find(transaction.GetProductId()) == -1)
            {
                productIds.Insert(transaction.GetProductId());
                
                // Also capture preset attachment configs if applicable
                TraderXPreset preset = transaction.GetPreset();
                if (preset && preset.attachments)
                {
                    for (int j = 0; j < preset.attachments.Count(); j++)
                    {
                        string attachmentId = preset.attachments.Get(j);
                        if (productIds.Find(attachmentId) == -1)
                        {
                            productIds.Insert(attachmentId);
                        }
                    }
                }
            }
        }
        
        // Capture product configurations for all involved products
        foreach (string productId : productIds)
        {
            TraderXProduct product = TraderXProductRepository.GetItemById(productId);
            if (product)
            {
                // Create a deep copy of the product configuration
                TraderXProduct productCopy = new TraderXProduct();
                productCopy.className = product.className;
                productCopy.coefficient = product.coefficient;
                productCopy.maxStock = product.maxStock;
                productCopy.tradeQuantity = product.tradeQuantity;
                productCopy.buyPrice = product.buyPrice;
                productCopy.sellPrice = product.sellPrice;
                productCopy.stockSettings = product.stockSettings;
                productCopy.productId = product.productId;
                
                // Copy attachments and variants arrays
                if (product.attachments)
                {
                    productCopy.attachments = new array<string>();
                    for (int k = 0; k < product.attachments.Count(); k++)
                    {
                        productCopy.attachments.Insert(product.attachments.Get(k));
                    }
                }
                
                if (product.variants)
                {
                    productCopy.variants = new array<string>();
                    for (int l = 0; l < product.variants.Count(); l++)
                    {
                        productCopy.variants.Insert(product.variants.Get(l));
                    }
                }
                
                productConfigurations.Insert(productCopy);
                GetTraderXLogger().LogDebug(string.Format("[DEBUG] Product config captured: %1 (buyPrice: %2, coefficient: %3, maxStock: %4)", 
                    productId, product.buyPrice, product.coefficient, product.maxStock));
            }
        }
    }

    /**
     * Load test case from JSON file using TraderXJsonLoader
     */
    static ref TraderXTransactionTestCase LoadFromFile(string filePath)
    {
        TraderXTransactionTestCase testCase;
        TraderXJsonLoader<TraderXTransactionTestCase>.LoadFromFile(filePath, testCase);
        return testCase;
    }
    
}
