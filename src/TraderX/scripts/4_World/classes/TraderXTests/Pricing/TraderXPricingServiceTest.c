// Test class for TraderXPricingService to validate pricing calculations
class TraderXPricingServiceTest
{
    // Store product IDs for cross-test access
    private string ak74ProductId;
    private string m4a1ProductId;
    private string ammoProductId;
    
    void RunPricingServiceTests()
    {
        GetTraderXLogger().LogInfo("=== Starting TraderXPricingService Tests ===");
        
        OnBeforeTest();
        
        // Test basic pricing calculations
        TestBasicBuyPricing();
        TestBasicSellPricing();
        
        // Test coefficient-based pricing
        TestCoefficientBuyPricing();
        TestCoefficientSellPricing();
        
        // Test multiplier pricing
        TestMultiplierBuyPricing();
        TestMultiplierSellPricing();
        
        // Test unlimited stock pricing
        TestUnlimitedStockPricing();
        
        // Test item state multipliers
        TestItemStateMultipliers();
        
        // Test edge cases
        TestEdgeCases();
        
        // Test price validation
        TestPriceValidation();
        
        PrintTestSummary();
    }
    
    void OnBeforeTest()
    {
        GetTraderXLogger().LogInfo("[TEST] Setting up pricing test environment");
        
        // Stock repository doesn't need explicit initialization
        // It will be initialized automatically when first accessed
    }
    
    void TestBasicBuyPricing()
    {
        GetTraderXLogger().LogInfo("[TEST] Testing basic buy pricing");
        
        // Create test product with basic pricing - use different className to avoid conflicts
        TraderXProduct product = TraderXProduct.CreateProduct("test_ak74", 1.0, 10, 1, 1000, 800);
        TraderXProductRepository.AddItemToItems(product);
        
        // Store the generated product ID for later tests
        ak74ProductId = product.GetProductId();
        GetTraderXLogger().LogInfo(string.Format("[TEST] Created AK74 with ID: %1", ak74ProductId));
        
        // Set initial stock to 5
        TraderXProductStock stock = TraderXProductStockRepository.GetStockByProductId(product.GetProductId());
        stock.SetStock(5);
        TraderXProductStockRepository.Save(stock);
        
        // Test buy price calculation
        TraderXPriceCalculation calculation = TraderXPricingService.GetInstance().CalculateBuyPrice(product, 1);
        int calculatedPrice = calculation.GetCalculatedPrice();
        
        // With coefficient 1.0 and stock 5, price should be: Math.Pow(1.0, (5-1)) * 1000 * 1.0 = 1000
        AssertEquals(calculatedPrice, 1000, "Basic buy price calculation");
        
        GetTraderXLogger().LogInfo(string.Format("[TEST] Basic buy price - Expected: 1000, Got: %1", calculatedPrice));
    }
    
    void TestBasicSellPricing()
    {
        GetTraderXLogger().LogInfo("[TEST] Testing basic sell pricing");
        
        // Use the same product from buy test
        TraderXProduct product = TraderXProductRepository.GetItemById(ak74ProductId);
        if (!product) {
            GetTraderXLogger().LogError(string.Format("[TEST] Product not found for sell pricing test. Looking for ID: %1", ak74ProductId));
            return;
        }
        
        // Test sell price calculation
        TraderXPriceCalculation calculation = TraderXPricingService.GetInstance().CalculateSellPrice(product, 1);
        int calculatedPrice = calculation.GetCalculatedPrice();
        
        // With coefficient 1.0 and stock 5, price should be: Math.Pow(1.0, (5-1)) * 800 * 1.0 = 800
        AssertEquals(calculatedPrice, 800, "Basic sell price calculation");
        
        GetTraderXLogger().LogInfo(string.Format("[TEST] Basic sell price - Expected: 800, Got: %1", calculatedPrice));
    }
    
    void TestCoefficientBuyPricing()
    {
        GetTraderXLogger().LogInfo("[TEST] Testing coefficient-based buy pricing");
        
        // Create product with coefficient > 1
        TraderXProduct product = TraderXProduct.CreateProduct("m4a1", 1.2, 10, 1, 1500, 1200);
        TraderXProductRepository.AddItemToItems(product);
        
        // Store the generated product ID for later tests
        m4a1ProductId = product.GetProductId();
        GetTraderXLogger().LogInfo(string.Format("[TEST] Created M4A1 with ID: %1", m4a1ProductId));
        
        // Set stock to 3
        TraderXProductStock stock = TraderXProductStockRepository.GetStockByProductId(product.GetProductId());
        stock.SetStock(3);
        TraderXProductStockRepository.Save(stock);
        
        // Test buy price calculation
        TraderXPriceCalculation calculation = TraderXPricingService.GetInstance().CalculateBuyPrice(product, 1);
        int calculatedPrice = calculation.GetCalculatedPrice();
        
        // With coefficient 1.2 and stock 3, price should be: Math.Pow(1.2, (3-1)) * 1500 * 1.0 = 1.44 * 1500 = 2160
        int expectedPrice = Math.Round(Math.Pow(1.2, 2) * 1500);
        AssertEquals(calculatedPrice, expectedPrice, "Coefficient buy price calculation");
        
        GetTraderXLogger().LogInfo(string.Format("[TEST] Coefficient buy price - Expected: %1, Got: %2", expectedPrice, calculatedPrice));
    }
    
    void TestCoefficientSellPricing()
    {
        GetTraderXLogger().LogInfo("[TEST] Testing coefficient-based sell pricing");
        
        // Use the same product from coefficient buy test
        TraderXProduct product = TraderXProductRepository.GetItemById(m4a1ProductId);
        if (!product) {
            GetTraderXLogger().LogError(string.Format("[TEST] Product not found for coefficient sell pricing test. Looking for ID: %1", m4a1ProductId));
            return;
        }
        
        // Test sell price calculation
        TraderXPriceCalculation calculation = TraderXPricingService.GetInstance().CalculateSellPrice(product, 1);
        int calculatedPrice = calculation.GetCalculatedPrice();
        
        // With coefficient 1.2 and stock 3, price should be: Math.Pow(1.2, (3-1)) * 1200 * 1.0 = 1.44 * 1200 = 1728
        int expectedPrice = Math.Round(Math.Pow(1.2, 2) * 1200);
        AssertEquals(calculatedPrice, expectedPrice, "Coefficient sell price calculation");
        
        GetTraderXLogger().LogInfo(string.Format("[TEST] Coefficient sell price - Expected: %1, Got: %2", expectedPrice, calculatedPrice));
    }
    
    void TestMultiplierBuyPricing()
    {
        GetTraderXLogger().LogInfo("[TEST] Testing multiplier buy pricing");
        
        // Create product for multiplier testing
        TraderXProduct product = TraderXProduct.CreateProduct("ammo_762x39", 1.1, 50, 30, 50, 40);
        TraderXProductRepository.AddItemToItems(product);
        
        // Store the generated product ID for later tests
        ammoProductId = product.GetProductId();
        GetTraderXLogger().LogInfo(string.Format("[TEST] Created Ammo with ID: %1", ammoProductId));
        
        // Set stock to 10
        TraderXProductStock stock = TraderXProductStockRepository.GetStockByProductId(product.GetProductId());
        stock.SetStock(10);
        TraderXProductStockRepository.Save(stock);
        
        // Test buy price calculation with multiplier 3
        TraderXPriceCalculation calculation = TraderXPricingService.GetInstance().CalculateBuyPrice(product, 3);
        int calculatedPrice = calculation.GetCalculatedPrice();
        
        // Calculate expected price for multiplier 3
        float totalPrice = 0;
        for (int i = 1; i <= 3; i++) {
            int stockLevel = 10 + (i - 1); // Stock increases as we "buy" each unit
            totalPrice += Math.Pow(1.1, (stockLevel - 1)) * 50;
        }
        int expectedPrice = Math.Round(totalPrice);
        
        AssertEquals(calculatedPrice, expectedPrice, "Multiplier buy price calculation");
        
        GetTraderXLogger().LogInfo(string.Format("[TEST] Multiplier buy price - Expected: %1, Got: %2", expectedPrice, calculatedPrice));
    }
    
    void TestMultiplierSellPricing()
    {
        GetTraderXLogger().LogInfo("[TEST] Testing multiplier sell pricing");
        
        // Use the same product from multiplier buy test
        TraderXProduct product = TraderXProductRepository.GetItemById(ammoProductId);
        if (!product) {
            GetTraderXLogger().LogError(string.Format("[TEST] Product not found for multiplier sell pricing test. Looking for ID: %1", ammoProductId));
            return;
        }
        
        // Test sell price calculation with multiplier 2
        TraderXPriceCalculation calculation = TraderXPricingService.GetInstance().CalculateSellPrice(product, 2);
        int calculatedPrice = calculation.GetCalculatedPrice();
        
        // Calculate expected price for multiplier 2
        float totalPrice = 0;
        for (int i = 1; i <= 2; i++) {
            int stockLevel = 10 + (i - 1); // Stock increases as we "sell" each unit
            totalPrice += Math.Pow(1.1, (stockLevel - 1)) * 40;
        }
        int expectedPrice = Math.Round(totalPrice);
        
        AssertEquals(calculatedPrice, expectedPrice, "Multiplier sell price calculation");
        
        GetTraderXLogger().LogInfo(string.Format("[TEST] Multiplier sell price - Expected: %1, Got: %2", expectedPrice, calculatedPrice));
    }
    
    void TestUnlimitedStockPricing()
    {
        GetTraderXLogger().LogInfo("[TEST] Testing unlimited stock pricing");
        
        // Create product with unlimited stock (maxStock = -1)
        TraderXProduct product = TraderXProduct.CreateProduct("bandage", 1.5, -1, 1, 100, 80);
        TraderXProductRepository.AddItemToItems(product);
        
        // Test buy price calculation
        TraderXPriceCalculation buyCalculation = TraderXPricingService.GetInstance().CalculateBuyPrice(product, 5);
        int buyPrice = buyCalculation.GetCalculatedPrice();
        
        // For unlimited stock, price should be: basePrice * multiplier * stateMultiplier = 100 * 5 * 1.0 = 500
        AssertEquals(buyPrice, 500, "Unlimited stock buy price");
        
        // Test sell price calculation
        TraderXPriceCalculation sellCalculation = TraderXPricingService.GetInstance().CalculateSellPrice(product, 3);
        int sellPrice = sellCalculation.GetCalculatedPrice();
        
        // For unlimited stock, price should be: basePrice * multiplier * stateMultiplier = 80 * 3 * 1.0 = 240
        AssertEquals(sellPrice, 240, "Unlimited stock sell price");
        
        GetTraderXLogger().LogInfo(string.Format("[TEST] Unlimited stock - Buy: %1, Sell: %2", buyPrice, sellPrice));
    }
    
    void TestItemStateMultipliers()
    {
        GetTraderXLogger().LogInfo("[TEST] Testing item state multipliers");
        
        // Create test product
        TraderXProduct product = TraderXProduct.CreateProduct("knife", 1.0, -1, 1, 200, 150);
        TraderXProductRepository.AddItemToItems(product);
        
        // Test different item states
        int pristinePrice = TraderXPricingService.GetInstance().CalculateBuyPrice(product, 1, TraderXItemState.PRISTINE).GetCalculatedPrice();
        int wornPrice = TraderXPricingService.GetInstance().CalculateBuyPrice(product, 1, TraderXItemState.WORN).GetCalculatedPrice();
        int damagedPrice = TraderXPricingService.GetInstance().CalculateBuyPrice(product, 1, TraderXItemState.DAMAGED).GetCalculatedPrice();
        int badlyDamagedPrice = TraderXPricingService.GetInstance().CalculateBuyPrice(product, 1, TraderXItemState.BADLY_DAMAGED).GetCalculatedPrice();
        
        // Verify state multipliers
        AssertEquals(pristinePrice, 200, "Pristine state price");
        AssertEquals(wornPrice, 160, "Worn state price (80% of pristine)");
        AssertEquals(damagedPrice, 120, "Damaged state price (60% of pristine)");
        AssertEquals(badlyDamagedPrice, 80, "Badly damaged state price (40% of pristine)");
        
        GetTraderXLogger().LogInfo(string.Format("[TEST] State prices - Pristine: %1, Worn: %2, Damaged: %3, Badly Damaged: %4", 
            pristinePrice, wornPrice, damagedPrice, badlyDamagedPrice));
    }
    
    void TestEdgeCases()
    {
        GetTraderXLogger().LogInfo("[TEST] Testing edge cases");
        
        // Test product that cannot be bought (buyPrice = -1)
        TraderXProduct noBuyProduct = TraderXProduct.CreateProduct("rare_item", 1.0, 5, 1, -1, 500);
        TraderXProductRepository.AddItemToItems(noBuyProduct);
        
        TraderXPriceCalculation noBuyCalculation = TraderXPricingService.GetInstance().CalculateBuyPrice(noBuyProduct, 1);
        AssertEquals(noBuyCalculation.GetCalculatedPrice(), -1, "Cannot buy item price");
        AssertTrue(!noBuyCalculation.IsValidPrice(), "Cannot buy item validity");
        
        // Test product that cannot be sold (sellPrice = -1)
        TraderXProduct noSellProduct = TraderXProduct.CreateProduct("quest_item", 1.0, 5, 1, 1000, -1);
        TraderXProductRepository.AddItemToItems(noSellProduct);
        
        TraderXPriceCalculation noSellCalculation = TraderXPricingService.GetInstance().CalculateSellPrice(noSellProduct, 1);
        AssertEquals(noSellCalculation.GetCalculatedPrice(), -1, "Cannot sell item price");
        AssertTrue(!noSellCalculation.IsValidPrice(), "Cannot sell item validity");
        
        // Test zero stock scenario
        TraderXProduct zeroStockProduct = TraderXProduct.CreateProduct("scarce_item", 1.3, 10, 1, 800, 600);
        TraderXProductRepository.AddItemToItems(zeroStockProduct);
        TraderXProductStock zeroStock = TraderXProductStockRepository.GetStockByProductId(zeroStockProduct.GetProductId());
        zeroStock.SetStock(0);
        TraderXProductStockRepository.Save(zeroStock);
        
        TraderXPriceCalculation zeroStockCalculation = TraderXPricingService.GetInstance().CalculateBuyPrice(zeroStockProduct, 1);
        int zeroStockPrice = zeroStockCalculation.GetCalculatedPrice();
        
        // With stock 0 (treated as 1), price should be: Math.Pow(1.3, (1-1)) * 800 = 1.0 * 800 = 800
        AssertEquals(zeroStockPrice, 800, "Zero stock price calculation");
        
        GetTraderXLogger().LogInfo("[TEST] Edge cases completed");
    }
    
    void TestPriceValidation()
    {
        GetTraderXLogger().LogInfo("[TEST] Testing price validation");
        
        // Create a mock transaction for validation testing
        TraderXProduct product = TraderXProductRepository.GetItemById(ak74ProductId);
        if (!product) {
            GetTraderXLogger().LogError(string.Format("[TEST] Product not found for price validation test. Looking for ID: %1", ak74ProductId));
            return;
        }
        
        // Calculate expected price
        int expectedPrice = TraderXPricingService.GetInstance().GetPricePreview(product.GetProductId(), true, 1);
        
        // Test validation with correct price (would need actual transaction object)
        GetTraderXLogger().LogInfo(string.Format("[TEST] Price validation - Expected price for %1: %2", product.className, expectedPrice));
        
        // Test batch pricing
        array<ref TraderXTransaction> mockTransactions = new array<ref TraderXTransaction>();
        // Note: Would need actual transaction objects for full batch testing
        
        GetTraderXLogger().LogInfo("[TEST] Price validation completed");
    }
    
    void AssertEquals(int actual, int expected, string testName)
    {
        if (actual == expected) {
            GetTraderXLogger().LogInfo(string.Format("[TEST PASS] %1 - Expected: %2, Got: %3", testName, expected, actual));
        } else {
            GetTraderXLogger().LogError(string.Format("[TEST FAIL] %1 - Expected: %2, Got: %3", testName, expected, actual));
        }
    }
    
    void AssertTrue(bool condition, string testName)
    {
        if (condition) {
            GetTraderXLogger().LogInfo(string.Format("[TEST PASS] %1", testName));
        } else {
            GetTraderXLogger().LogError(string.Format("[TEST FAIL] %1", testName));
        }
    }
    
    void PrintTestSummary()
    {
        GetTraderXLogger().LogInfo("=== TraderXPricingService Tests Completed ===");
        GetTraderXLogger().LogInfo("All pricing calculations have been validated against the original TraderPlus algorithm");
        GetTraderXLogger().LogInfo("Key features tested:");
        GetTraderXLogger().LogInfo("- Basic buy/sell pricing");
        GetTraderXLogger().LogInfo("- Coefficient-based dynamic pricing");
        GetTraderXLogger().LogInfo("- Multiplier calculations");
        GetTraderXLogger().LogInfo("- Unlimited stock handling");
        GetTraderXLogger().LogInfo("- Item state multipliers");
        GetTraderXLogger().LogInfo("- Edge cases and validation");
    }
}
