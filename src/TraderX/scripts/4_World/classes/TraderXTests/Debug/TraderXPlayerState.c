/**
 * TraderXPlayerState - Captures complete player state for test case generation
 * Records money, inventory items, and equipment for transaction debugging
 */
class TraderXPlayerState
{
    int totalMoney;
    ref array<string> currencyTypes;
    ref array<ref TraderXTestInventoryItem> inventoryItems;
    ref array<ref TraderXTestInventoryItem> equipmentItems;
    
    void TraderXPlayerState()
    {
        currencyTypes = new array<string>();
        inventoryItems = new array<ref TraderXTestInventoryItem>();
        equipmentItems = new array<ref TraderXTestInventoryItem>();
    }
    
    void CaptureState(PlayerBase player, int traderId)
    {
        if (!player) return;
        
        // Get NPC currencies for money calculation
        TraderXNpc npc = GetTraderXModule().GetSettings().GetNpcById(traderId);
        if (npc)
        {
            currencyTypes = npc.GetCurrenciesAccepted();
            totalMoney = TraderXCurrencyService.GetInstance().GetPlayerMoneyFromAllCurrency(player, currencyTypes);
        }
        
        // Capture inventory items
        CaptureInventoryItems(player);
        CaptureEquipmentItems(player);
    }
    
    private void CaptureInventoryItems(PlayerBase player)
    {
        inventoryItems.Clear();
        
        array<EntityAI> items = new array<EntityAI>();
        player.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, items);
        
        foreach (EntityAI item : items)
        {
            if (item && item != player)
            {
                TraderXTestInventoryItem invItem = new TraderXTestInventoryItem();
                invItem.className = item.GetType();
                invItem.quantity = TraderXQuantityManager.GetItemAmount(item);
                
                // Get network ID for items that might be sold
                int lowId, highId;
                item.GetNetworkID(lowId, highId);
                invItem.networkIdLow = lowId;
                invItem.networkIdHigh = highId;
                
                // Check if it's equipment or cargo
                if (IsEquipmentSlot(item, player))
                {
                    equipmentItems.Insert(invItem);
                }
                else
                {
                    inventoryItems.Insert(invItem);
                }
            }
        }
    }
    
    private void CaptureEquipmentItems(PlayerBase player)
    {
        // Equipment items are already captured in CaptureInventoryItems
        // This method is kept for potential future expansion
    }
    
    private bool IsEquipmentSlot(EntityAI item, PlayerBase player)
    {
        // Check if item is in equipment slots (clothing, etc.)
        string itemType = item.GetType();
        return (itemType.Contains("Hood") || itemType.Contains("Shirt") || itemType.Contains("Vest") || itemType.Contains("Pants") || itemType.Contains("Shoes") || itemType.Contains("Bag") || itemType.Contains("Helmet") || itemType.Contains("Gloves"));
    }
    
    string ToFormattedString()
    {
        string output = "";
        output += string.Format("Money: %1 (currencies: %2)\n", totalMoney, currencyTypes.Count());
        
        output += string.Format("Equipment Items (%1):\n", equipmentItems.Count());
        foreach (TraderXTestInventoryItem item : equipmentItems)
        {
            output += string.Format("  - %1 x%2 (net: %3/%4)\n", item.className, item.quantity, item.networkIdLow, item.networkIdHigh);
        }
        
        output += string.Format("Inventory Items (%1):\n", inventoryItems.Count());
        foreach (TraderXTestInventoryItem itm : inventoryItems)
        {
            output += string.Format("  - %1 x%2 (net: %3/%4)\n", itm.className, itm.quantity, itm.networkIdLow, itm.networkIdHigh);
        }
        
        return output;
    }
    
    /**
     * Save player state to JSON file using TraderXJsonLoader
     */
    void SaveToFile(string filePath)
    {
        TraderXJsonLoader<TraderXPlayerState>.SaveToFile(filePath, this);
    }
    
    /**
     * Load player state from JSON file using TraderXJsonLoader
     */
    static ref TraderXPlayerState LoadFromFile(string filePath)
    {
        TraderXPlayerState playerState;
        TraderXJsonLoader<TraderXPlayerState>.LoadFromFile(filePath, playerState);
        return playerState;
    }
    
    
    private bool IsCurrencyItem(string className)
    {
        // Check if item is a currency item
        foreach (string currency : currencyTypes)
        {
            if (className.Contains(currency) || className.Contains("Money") || className.Contains("Cash"))
            {
                return true;
            }
        }
        return false;
    }
    
    private string GenerateItemVarName(string className)
    {
        // Generate a valid variable name from class name
        string varName = className;
        varName.Replace("_", "");
        varName.Replace("-", "");
        return varName.Substring(0, Math.Min(varName.Length(), 15)); // Limit length
    }
}

/**
 * Helper class to store inventory item information
 */
class TraderXTestInventoryItem
{
    string className;
    int quantity;
    int networkIdLow;
    int networkIdHigh;
    
    void TraderXTestInventoryItem()
    {
        quantity = 1;
        networkIdLow = 0;
        networkIdHigh = 0;
    }
}
