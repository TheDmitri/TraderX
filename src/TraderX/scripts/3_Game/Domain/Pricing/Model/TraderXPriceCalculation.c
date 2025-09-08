// Domain value object for price calculations
class TraderXPriceCalculation
{
    private int basePrice;
    private float coefficient;
    private int stockQuantity;
    private int multiplier;
    private float stateMultiplier;
    private int calculatedPrice;
    private bool isUnlimitedStock;
    
    void TraderXPriceCalculation(int basePrice, float coefficient, int stockQuantity, int multiplier, float stateMultiplier, bool isUnlimitedStock)
    {
        this.basePrice = basePrice;
        this.coefficient = coefficient;
        this.stockQuantity = stockQuantity;
        this.multiplier = multiplier;
        this.stateMultiplier = stateMultiplier;
        this.isUnlimitedStock = isUnlimitedStock;
        this.calculatedPrice = CalculatePrice();
    }
    
    static TraderXPriceCalculation CreateBuyCalculation(int itemBasePrice, float priceCoefficient, int currentStock, int itemMultiplier, float itemStateMultiplier, bool unlimitedStock)
    {
        return new TraderXPriceCalculation(itemBasePrice, priceCoefficient, currentStock, itemMultiplier, itemStateMultiplier, unlimitedStock);
    }
    
    static TraderXPriceCalculation CreateSellCalculation(int itemBasePrice, float priceCoefficient, int currentStock, int itemMultiplier, float itemStateMultiplier, bool unlimitedStock)
    {
        TraderXPriceCalculation calc = new TraderXPriceCalculation(itemBasePrice, priceCoefficient, currentStock, itemMultiplier, itemStateMultiplier, unlimitedStock);
        calc.calculatedPrice = calc.CalculateSellPrice();
        return calc;
    }
    
    private int CalculatePrice()
    {
        // Return -1 for invalid prices (item cannot be traded)
        if (basePrice == -1)
            return -1;
            
        // For unlimited stock OR static pricing (coefficient=1), use simple multiplication
        if (isUnlimitedStock || coefficient == 1.0)
            return Math.Round(basePrice * multiplier * stateMultiplier);
        
        // Apply coefficient-based pricing for limited stock with dynamic pricing
        float totalPrice = 0;
        int currentStockForCalculation = stockQuantity;
        
        for (int i = 1; i <= multiplier; i++)
        {
            int stockLevel = currentStockForCalculation;
            if (stockLevel == 0) stockLevel = 1; // Prevent division by zero in coefficient calculation
            
            float priceForThisUnit = Math.Pow(coefficient, (stockLevel - 1)) * basePrice * stateMultiplier;
            totalPrice += priceForThisUnit;
            
            currentStockForCalculation++;
        }
        
        int finalPrice = Math.Round(totalPrice);
        if (finalPrice == 0) finalPrice = 1; // Minimum price of 1
        
        return finalPrice;
    }
    
    private int CalculateSellPrice()
    {
        // Return -1 for invalid prices (item cannot be traded)
        if (basePrice == -1)
            return -1;
            
        // For unlimited stock OR static pricing (coefficient=1), use simple multiplication
        if (isUnlimitedStock || coefficient == 1.0)
            return Math.Round(basePrice * multiplier * stateMultiplier);
        
        // For sell transactions with multiplier > 1, calculate progressive pricing
        // Each item sold increases trader's stock, making subsequent items worth less
        float totalPrice = 0;
        int currentStockForCalculation = stockQuantity;
        
        for (int i = 0; i < multiplier; i++)
        {
            int stockLevel = currentStockForCalculation + i;
            if (stockLevel <= 0) stockLevel = 1; // Prevent division by zero
            
            float priceForThisUnit = Math.Pow(coefficient, (stockLevel - 1)) * basePrice * stateMultiplier;
            totalPrice += priceForThisUnit;
        }
        
        int finalPrice = Math.Round(totalPrice);
        if (finalPrice == 0) finalPrice = 1; // Minimum price of 1
        
        return finalPrice;
    }
    
    int GetCalculatedPrice()
    {
        return calculatedPrice;
    }
    
    bool IsValidPrice()
    {
        return calculatedPrice >= 0;
    }
    
    bool IsFreeItem()
    {
        return calculatedPrice == 0;
    }
    
    int GetBasePrice()
    {
        return basePrice;
    }
    
    float GetCoefficient()
    {
        return coefficient;
    }
    
    int GetStockQuantity()
    {
        return stockQuantity;
    }
    
    int GetMultiplier()
    {
        return multiplier;
    }
    
    float GetStateMultiplier()
    {
        return stateMultiplier;
    }
    
    // Get individual item prices for progressive pricing display
    array<int> GetProgressivePrices()
    {
        array<int> prices = new array<int>();
        
        if (basePrice == -1 || isUnlimitedStock) {
            // For unlimited stock or invalid prices, all items have the same price
            int unitPrice = Math.Round(basePrice * stateMultiplier);
            for (int i = 0; i < multiplier; i++) {
                prices.Insert(unitPrice);
            }
            return prices;
        }
        
        // Calculate individual prices for each item in the multiplier
        int currentStockForCalculation = stockQuantity;
        
        for (int j = 0; j < multiplier; j++)
        {
            int stockLevel = currentStockForCalculation + j;
            if (stockLevel <= 0) stockLevel = 1;
            
            float priceForThisUnit = Math.Pow(coefficient, (stockLevel - 1)) * basePrice * stateMultiplier;
            int roundedPrice = Math.Round(priceForThisUnit);
            if (roundedPrice == 0) roundedPrice = 1;
            
            prices.Insert(roundedPrice);
        }
        
        return prices;
    }
}
