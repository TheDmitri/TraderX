// Service for managing dynamic pricing during checkout with pre-stock calculations
class TraderXCheckoutPricingService
{
    private static ref TraderXCheckoutPricingService m_instance;
    private ref map<string, int> m_checkoutStockChanges; // productId -> quantity in checkout
    private ref array<string> m_checkoutOrder; // Order items were added to checkout (using unique keys)
    private ref map<string, string> m_itemKeyToProductId; // unique key -> productId mapping
    
    void TraderXCheckoutPricingService()
    {
        m_checkoutStockChanges = new map<string, int>();
        m_checkoutOrder = new array<string>();
        m_itemKeyToProductId = new map<string, string>();
    }
    
    static TraderXCheckoutPricingService GetInstance()
    {
        if (!m_instance) {
            m_instance = new TraderXCheckoutPricingService();
        }
        return m_instance;
    }
    
    // Add item to checkout with unique identifier for individual items
    void AddItemToCheckout(string productId, int quantity, string uniqueKey = "")
    {
        // Generate unique key if not provided (for backward compatibility)
        if (uniqueKey == "") {
            uniqueKey = productId + "_" + m_checkoutOrder.Count();
        }
        
        // Track the unique item in checkout order
        m_checkoutOrder.Insert(uniqueKey);
        m_itemKeyToProductId.Set(uniqueKey, productId);
        
        // Update total quantity for this product type
        if (!m_checkoutStockChanges.Contains(productId)) {
            m_checkoutStockChanges.Set(productId, 0);
        }
        
        int currentCheckoutQty = m_checkoutStockChanges.Get(productId);
        m_checkoutStockChanges.Set(productId, currentCheckoutQty + quantity);
        
        // Notify price updates for items of the same type that are already in checkout
        NotifyPriceUpdatesForItemType(productId);
    }
    
    // Remove item from checkout and update pricing
    void RemoveItemFromCheckout(string productId, int quantity)
    {
        if (!m_checkoutStockChanges.Contains(productId)) {
            return;
        }
        
        int currentCheckoutQty = m_checkoutStockChanges.Get(productId);
        int newQty = Math.Max(0, currentCheckoutQty - quantity);
        
        if (newQty == 0) {
            m_checkoutStockChanges.Remove(productId);
        } else {
            m_checkoutStockChanges.Set(productId, newQty);
        }
        
        NotifyPriceUpdatesForItemType(productId);
    }
    
    // Clear all checkout items
    void ClearCheckout()
    {
        m_checkoutStockChanges.Clear();
        m_checkoutOrder.Clear();
        // Notify all items to refresh their prices
        TraderXProduct.Event_OnCheckoutPricingChanged.Invoke();
    }
    
    // Get the effective stock level for pricing calculations based on checkout order
    int GetEffectiveStockLevelForItem(string productId)
    {
        int currentStock = TraderXProductStockRepository.GetStockAmount(productId);
        
        // For backward compatibility, use the first occurrence of this productId
        string targetKey = "";
        for (int i = 0; i < m_checkoutOrder.Count(); i++) {
            string key = m_checkoutOrder.Get(i);
            if (m_itemKeyToProductId.Contains(key) && m_itemKeyToProductId.Get(key) == productId) {
                targetKey = key;
                break;
            }
        }
        
        if (targetKey == "") {
            return currentStock; // Item not in checkout yet
        }
        
        return GetEffectiveStockLevelForItemKey(targetKey);
    }
    
    // Get the effective stock level for a specific item instance by its unique key
    int GetEffectiveStockLevelForItemKey(string itemKey)
    {
        if (!m_itemKeyToProductId.Contains(itemKey)) {
            GetTraderXLogger().LogDebug(string.Format("[CHECKOUT_PRICING] Invalid item key: %1", itemKey));
            return 0; // Invalid key
        }
        
        string productId = m_itemKeyToProductId.Get(itemKey);
        int currentStock = TraderXProductStockRepository.GetStockAmount(productId);
        
        // Find position of this specific item in checkout order
        int itemPosition = m_checkoutOrder.Find(itemKey);
        if (itemPosition == -1) {
            GetTraderXLogger().LogDebug(string.Format("[CHECKOUT_PRICING] Item not in checkout: %1", itemKey));
            return currentStock; // Item not in checkout yet
        }
        
        // Count items of the same type that were added before this one
        int precedingQuantity = 0;
        TraderXProduct product = TraderXProductRepository.GetItemById(productId);
        if (!product) return currentStock;
        
        for (int i = 0; i < itemPosition; i++) {
            string precedingKey = m_checkoutOrder.Get(i);
            if (m_itemKeyToProductId.Contains(precedingKey)) {
                string precedingProductId = m_itemKeyToProductId.Get(precedingKey);
                TraderXProduct precedingProduct = TraderXProductRepository.GetItemById(precedingProductId);
                
                // Only count items of the same type (className)
                if (precedingProduct && precedingProduct.className == product.className) {
                    precedingQuantity += 1; // Each item in checkout order represents 1 unit
                    GetTraderXLogger().LogDebug(string.Format("[CHECKOUT_PRICING] Found preceding item: %1 (type: %2)", precedingKey, precedingProduct.className));
                }
            }
        }
        
        int effectiveStock = currentStock + precedingQuantity;
        GetTraderXLogger().LogDebug(string.Format("[CHECKOUT_PRICING] ItemKey: %1, Position: %2, CurrentStock: %3, PrecedingQty: %4, EffectiveStock: %5", itemKey, itemPosition, currentStock, precedingQuantity, effectiveStock));
        
        return effectiveStock;
    }
    
    // Calculate sell price with pre-stock consideration
    TraderXPriceCalculation CalculateSellPriceWithCheckout(TraderXProduct product, int multiplier = 1, int itemState = TraderXItemState.PRISTINE)
    {
        if (!product || !product.CanBeSold()) {
            return new TraderXPriceCalculation(-1, 1.0, 0, multiplier, 1.0, true);
        }
        
        // Get effective stock level including items already in checkout based on order
        int effectiveStock = GetEffectiveStockLevelForItem(product.GetProductId());
        float stateMultiplier = TraderXItemState.GetStateMultiplier(itemState);
        bool isUnlimitedStock = product.IsStockUnlimited();
        
        GetTraderXLogger().LogDebug(string.Format("[CHECKOUT_PRICING] Product: %1, EffectiveStock: %2, Multiplier: %3", product.className, effectiveStock, multiplier));
        
        return TraderXPriceCalculation.CreateSellCalculation(product.sellPrice, product.coefficient, effectiveStock, multiplier, stateMultiplier, isUnlimitedStock);
    }
    
    // Calculate sell price for a specific item instance using its unique key
    TraderXPriceCalculation CalculateSellPriceWithCheckoutForItem(TraderXProduct product, string itemKey, int multiplier = 1, int itemState = TraderXItemState.PRISTINE)
    {
        if (!product || !product.CanBeSold()) {
            return new TraderXPriceCalculation(-1, 1.0, 0, multiplier, 1.0, true);
        }
        
        // Get effective stock level for this specific item instance
        int effectiveStock = GetEffectiveStockLevelForItemKey(itemKey);
        float stateMultiplier = TraderXItemState.GetStateMultiplier(itemState);
        bool isUnlimitedStock = product.IsStockUnlimited();
        
        GetTraderXLogger().LogDebug(string.Format("[CHECKOUT_PRICING] Item: %1 (Key: %2), EffectiveStock: %3, Multiplier: %4", product.className, itemKey, effectiveStock, multiplier));
        
        return TraderXPriceCalculation.CreateSellCalculation(product.sellPrice, product.coefficient, effectiveStock, multiplier, stateMultiplier, isUnlimitedStock);
    }
    
    // Calculate preset sell price with pre-stock consideration
    TraderXPriceCalculation CalculatePresetSellPriceWithCheckout(TraderXPreset preset, int multiplier = 1, int itemState = TraderXItemState.PRISTINE)
    {
        if (!preset) {
            return new TraderXPriceCalculation(-1, 1.0, 0, multiplier, 1.0, true);
        }
        
        // Get the main product
        TraderXProduct mainProduct = TraderXProductRepository.GetItemById(preset.productId);
        if (!mainProduct || !mainProduct.CanBeSold()) {
            return new TraderXPriceCalculation(-1, 1.0, 0, multiplier, 1.0, true);
        }
        
        // Get effective stock level for the main product (presets are sold as units of the main item)
        int effectiveStock = GetEffectiveStockLevelForItem(preset.productId);
        float stateMultiplier = TraderXItemState.GetStateMultiplier(itemState);
        bool isUnlimitedStock = mainProduct.IsStockUnlimited();
        
        // Calculate preset unit price (main item + attachments) at pristine condition
        int presetUnitPrice = TraderXPresetsService.GetInstance().CalculateTotalPricePreset(preset);
        
        // Apply progressive pricing using the preset unit price as base
        int totalPrice = 0;
        for (int i = 0; i < multiplier; i++) {
            int stockLevel = effectiveStock + i;
            float coefficient = mainProduct.coefficient;
            float priceForThisUnit;
            
            if (isUnlimitedStock || coefficient == 1.0) {
                priceForThisUnit = presetUnitPrice * stateMultiplier;
            } else {
                priceForThisUnit = Math.Pow(coefficient, (stockLevel - 1)) * presetUnitPrice * stateMultiplier;
            }
            totalPrice += priceForThisUnit;
        }
        
        GetTraderXLogger().LogDebug(string.Format("[CHECKOUT_PRICING] Preset: %1, EffectiveStock: %2, Multiplier: %3, TotalPrice: %4", preset.presetName, effectiveStock, multiplier, totalPrice));
        
        return new TraderXPriceCalculation(totalPrice, mainProduct.coefficient, effectiveStock, multiplier, stateMultiplier, isUnlimitedStock);
    }
    
    // Calculate preset sell price for a specific item instance using its unique key
    TraderXPriceCalculation CalculatePresetSellPriceWithCheckoutForItem(TraderXPreset preset, string itemKey, int multiplier = 1, int itemState = TraderXItemState.PRISTINE)
    {
        if (!preset) {
            return new TraderXPriceCalculation(-1, 1.0, 0, multiplier, 1.0, true);
        }
        
        // Get the main product
        TraderXProduct mainProduct = TraderXProductRepository.GetItemById(preset.productId);
        if (!mainProduct || !mainProduct.CanBeSold()) {
            return new TraderXPriceCalculation(-1, 1.0, 0, multiplier, 1.0, true);
        }
        
        // Get effective stock level for this specific item instance
        int effectiveStock = GetEffectiveStockLevelForItemKey(itemKey);
        float stateMultiplier = TraderXItemState.GetStateMultiplier(itemState);
        bool isUnlimitedStock = mainProduct.IsStockUnlimited();
        
        // Calculate preset unit price (main item + attachments) at pristine condition
        int presetUnitPrice = TraderXPresetsService.GetInstance().CalculateTotalPricePreset(preset);
        
        // Apply progressive pricing using the preset unit price as base
        int totalPrice = 0;
        for (int i = 0; i < multiplier; i++) {
            int stockLevel = effectiveStock + i;
            float coefficient = mainProduct.coefficient;
            float priceForThisUnit;
            
            if (isUnlimitedStock || coefficient == 1.0) {
                priceForThisUnit = presetUnitPrice * stateMultiplier;
            } else {
                priceForThisUnit = Math.Pow(coefficient, (stockLevel - 1)) * presetUnitPrice * stateMultiplier;
            }
            totalPrice += priceForThisUnit;
        }
        
        GetTraderXLogger().LogDebug(string.Format("[CHECKOUT_PRICING] Preset Item: %1 (Key: %2), EffectiveStock: %3, Multiplier: %4, TotalPrice: %5", preset.presetName, itemKey, effectiveStock, multiplier, totalPrice));
        
        return new TraderXPriceCalculation(totalPrice, mainProduct.coefficient, effectiveStock, multiplier, stateMultiplier, isUnlimitedStock);
    }
    
    // Get checkout quantity for a specific product
    int GetCheckoutQuantity(string productId)
    {
        if (m_checkoutStockChanges.Contains(productId)) {
            return m_checkoutStockChanges.Get(productId);
        }
        return 0;
    }
    
    // Get all items of the same type (className) that are in checkout
    array<string> GetCheckoutItemsOfType(string className)
    {
        array<string> items = new array<string>();
        
        foreach (string productId, int quantity : m_checkoutStockChanges) {
            TraderXProduct product = TraderXProductRepository.GetItemById(productId);
            if (product && product.className == className) {
                items.Insert(productId);
            }
        }
        
        return items;
    }
    
    private void NotifyPriceUpdatesForItemType(string productId)
    {
        TraderXProduct product = TraderXProductRepository.GetItemById(productId);
        if (!product) return;
        
        GetTraderXLogger().LogDebug(string.Format("[CHECKOUT_PRICING] Notifying price updates for product type: %1", product.className));
        
        // Notify all checkout cards to refresh their pricing
        TraderXProduct.Event_OnCheckoutPricingChanged.Invoke();
    }
}
