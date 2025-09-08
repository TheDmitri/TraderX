// Application service for handling dynamic pricing calculations
class TraderXPricingService
{
    static ref TraderXPricingService m_instance;
    
    static TraderXPricingService GetInstance()
    {
        if (!m_instance)
            m_instance = new TraderXPricingService();
        return m_instance;
    }
    
    // Calculate buy price for a product based on current stock and multiplier
    TraderXPriceCalculation CalculateBuyPrice(TraderXProduct product, int multiplier = 1, int itemState = TraderXItemState.PRISTINE)
    {
        if (!product || !product.CanBeBought())
        {
            return new TraderXPriceCalculation(-1, 1.0, 0, multiplier, 1.0, true);
        }
        
        int currentStock = GetCurrentStock(product.GetProductId());
        float stateMultiplier = TraderXItemState.GetStateMultiplier(itemState);
        bool isUnlimitedStock = product.IsStockUnlimited();
        
        GetTraderXLogger().LogInfo(string.Format("[PRICING] CalculateBuyPrice - Product: %1 (ID: %2), BasePrice: %3, Coefficient: %4, Stock: %5, Multiplier: %6, StateMultiplier: %7, IsUnlimited: %8", product.className, product.GetProductId(), product.buyPrice, product.coefficient, currentStock, multiplier, stateMultiplier, isUnlimitedStock));
        
        return TraderXPriceCalculation.CreateBuyCalculation(product.buyPrice, product.coefficient, currentStock, multiplier, stateMultiplier, isUnlimitedStock);
    }
    
    // Calculate sell price for a product based on current stock and multiplier
    TraderXPriceCalculation CalculateSellPrice(TraderXProduct product, int multiplier = 1, int itemState = TraderXItemState.PRISTINE)
    {
        if (!product || !product.CanBeSold())
        {
            return new TraderXPriceCalculation(-1, 1.0, 0, multiplier, 1.0, true);
        }
        
        int currentStock = GetCurrentStock(product.GetProductId());
        float stateMultiplier = TraderXItemState.GetStateMultiplier(itemState);
        bool isUnlimitedStock = product.IsStockUnlimited();
        
        GetTraderXLogger().LogDebug(string.Format("[PRICING] CalculateSellPrice - Product: %1, BasePrice: %2, Coefficient: %3, Stock: %4, Multiplier: %5, StateMultiplier: %6", product.className, product.sellPrice, product.coefficient, currentStock, multiplier, stateMultiplier));
        
        return TraderXPriceCalculation.CreateSellCalculation(product.sellPrice, product.coefficient, currentStock, multiplier, stateMultiplier, isUnlimitedStock);
    }
    
    // Calculate price for a transaction request
    TraderXPriceCalculation CalculateTransactionPrice(TraderXTransaction transaction)
    {
        if (!transaction || !transaction.IsValid())
        {
            return new TraderXPriceCalculation(-1, 1.0, 0, 1, 1.0, true);
        }
        
        TraderXProduct product = TraderXProductRepository.GetItemById(transaction.GetProductId());
        if (!product)
        {
            return new TraderXPriceCalculation(-1, 1.0, 0, 1, 1.0, true);
        }
        
        int multiplier = transaction.GetMultiplier();
        int itemState = TraderXItemState.PRISTINE; // Default state for new transactions
        
        if (transaction.IsBuy())
        {
            return CalculateBuyPrice(product, multiplier, itemState);
        }
        else
        {
            return CalculateSellPrice(product, multiplier, itemState);
        }
    }
    
    // Validate if a transaction price matches the expected calculated price
    bool ValidateTransactionPrice(TraderXTransaction transaction)
    {
        TraderXPriceCalculation calculation = CalculateTransactionPrice(transaction);
        int expectedPrice = calculation.GetCalculatedPrice();
        int transactionPrice = transaction.GetTotalPrice().GetAmount();
        
        bool isValid = (expectedPrice == transactionPrice);
        
        if (!isValid)
        {
            GetTraderXLogger().LogWarning(string.Format("[PRICING] Price validation failed - Expected: %1, Transaction: %2, Product: %3", expectedPrice, transactionPrice, transaction.GetProductId()));
        }
        
        return isValid;
    }
    
    // Get current stock for a product
    private int GetCurrentStock(string productId)
    {
        TraderXProductStock stockInfo = TraderXProductStockRepository.GetStockByProductId(productId);
        if (!stockInfo)
            return 0;
        
        return stockInfo.GetStock();
    }
    
    // Calculate price for multiple items (used for batch operations)
    int CalculateBatchPrice(array<ref TraderXTransaction> transactions)
    {
        int totalPrice = 0;
        
        for (int i = 0; i < transactions.Count(); i++)
        {
            TraderXPriceCalculation calculation = CalculateTransactionPrice(transactions[i]);
            if (calculation.IsValidPrice())
            {
                totalPrice += calculation.GetCalculatedPrice();
            }
            else
            {
                GetTraderXLogger().LogError(string.Format("[PRICING] Invalid price in batch calculation for transaction: %1", transactions[i].GetTransactionId()));
                return -1; // Invalid batch
            }
        }
        
        return totalPrice;
    }
    
    // Get price preview for UI display (without processing transaction)
    int GetPricePreview(string productId, bool isBuyTransaction, int multiplier = 1, int itemState = TraderXItemState.PRISTINE)
    {
        TraderXProduct product = TraderXProductRepository.GetItemById(productId);
        if (!product)
            return -1;
        
        TraderXPriceCalculation calculation;
        if (isBuyTransaction)
        {
            calculation = CalculateBuyPrice(product, multiplier, itemState);
        }
        else
        {
            calculation = CalculateSellPrice(product, multiplier, itemState);
        }
        
        return calculation.GetCalculatedPrice();
    }
}
