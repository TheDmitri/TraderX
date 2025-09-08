class TraderXTransaction
{
    private ref TraderXTransactionId transactionId;
    private ref TraderXTransactionType transactionType;
    private string productId;
    private string parentItemId;
    private ref TraderXPreset preset;
    private int multiplier;
    private ref TraderXTransactionPrice unitPrice;
    private ref TraderXTransactionPrice totalPrice;
    private ref TraderXNetworkIdentifier networkId;
    private int depth;
    private int traderId;
    
    void TraderXTransaction(TraderXTransactionId id, TraderXTransactionType type, string _id, string _parentItemId, int multi, TraderXTransactionPrice price, TraderXNetworkIdentifier netId = null, int itemDepth = 0, int npcId = 0, TraderXPreset tpPreset = null)
    {
        transactionId = id;
        transactionType = type;
        productId = _id;
        parentItemId = _parentItemId;
        multiplier = multi;
        unitPrice = price;
        totalPrice = price.Multiply(multiplier);
        preset = tpPreset;
        if(netId){
            networkId = netId;
        }
        else{
            networkId = TraderXNetworkIdentifier.CreateEmpty();
        }
        depth = itemDepth;
        traderId = npcId;
    }
    
    static TraderXTransaction CreateBuyTransaction(TraderXProduct item, int multi, int price, int npcId, TraderXPreset tpPreset = null)
    {
        GetTraderXLogger().LogInfo("CreateBuyTransaction : id " + item.productId + " " + multi + " " + price + " " + npcId);
        TraderXTransactionId id = TraderXTransactionId.Generate();
        TraderXTransactionType type = TraderXTransactionType.CreateBuy();
        TraderXTransactionPrice priceObj = TraderXTransactionPrice.CreateFromAmount(price);
        TraderXNetworkIdentifier netId = TraderXNetworkIdentifier.CreateEmpty();
        return new TraderXTransaction(id, type, item.productId, item.GetParentEntityItemId(), multi, priceObj, netId, 0, npcId, tpPreset);
    }
    
    static TraderXTransaction CreateSellTransaction(TraderXProduct item, int multi, int price, int npcId)
    {
        GetTraderXLogger().LogInfo(string.Format("CreateSellTransaction - item=%1, multi=%2, price=%3, npcId=%4", item.productId, multi, price, npcId));
        TraderXTransactionId id = TraderXTransactionId.Generate();
        TraderXTransactionType type = TraderXTransactionType.CreateSell();
        TraderXTransactionPrice priceObj = TraderXTransactionPrice.CreateFromAmount(price);
        TraderXNetworkIdentifier netId = TraderXNetworkIdentifier.CreateFromIds(item.GetPlayerItem().GetNetworkIdLow(), item.GetPlayerItem().GetNetworkIdHigh());
        TraderXTransaction transaction = new TraderXTransaction(id, type, item.productId, string.Empty, multi, priceObj, netId, item.GetPlayerItem().GetDepth(), npcId);
        GetTraderXLogger().LogInfo(string.Format("CreateSellTransaction - Created transaction, IsValid=%1", transaction.IsValid()));
        return transaction;
    }
    
    // Getters
    TraderXTransactionId GetTransactionId() { return transactionId; }
    TraderXTransactionType GetTransactionType() { return transactionType; }
    string GetProductId() { return productId; }
    string GetParentItemId() { return parentItemId; }
    int GetMultiplier() { return multiplier; }
    TraderXTransactionPrice GetUnitPrice() { return unitPrice; }
    TraderXTransactionPrice GetTotalPrice() { return totalPrice; }
    TraderXNetworkIdentifier GetNetworkId() { return networkId; }
    int GetDepth() { return depth; }
    int GetTraderId() { return traderId; }
    TraderXPreset GetPreset() { return preset; }
    
    // Query methods
    bool IsBuy() { return transactionType.IsBuy(); }
    bool IsSell() { return transactionType.IsSell(); }
    bool HasNetworkId() { return !networkId.IsEmpty(); }
    bool IsValid() {
        bool hasId = transactionId != null;
        bool hasType = transactionType != null;
        bool hasValidMultiplier = multiplier > 0;
        bool hasValidPrice = unitPrice && unitPrice.IsValidPrice();
        
        GetTraderXLogger().LogInfo(string.Format("IsValid - hasId: %1, hasType: %2, multiplier: %3 (valid: %4), price valid: %5", 
            hasId, hasType, multiplier, hasValidMultiplier, hasValidPrice));
            
        if (!hasId) GetTraderXLogger().LogError("Transaction validation failed: missing transaction ID");
        if (!hasType) GetTraderXLogger().LogError("Transaction validation failed: missing transaction type");
        if (!hasValidMultiplier) GetTraderXLogger().LogError("Transaction validation failed: invalid multiplier: " + multiplier);
        if (!hasValidPrice) GetTraderXLogger().LogError("Transaction validation failed: invalid price");
        
        return hasId && hasType && hasValidMultiplier && hasValidPrice;
    }
    
    string ToStringFormatted()
    {
        string result = "";
        result += "{\n";
        result += "  transactionId: " + transactionId.ToString() + "\n";
        result += "  type: " + Ternary<string>.If(IsBuy(), "BUY", "SELL") + "\n";
        result += "  productId: " + productId + "\n";
        result += "  parentItemId: " + parentItemId + "\n";
        result += "  multiplier: " + Ternary<string>.If(IsBuy(), multiplier.ToString(), "-" + multiplier.ToString()) + "\n";
        result += "  unitPrice: " + unitPrice.ToStringFormatted() + "\n";
        result += "  totalPrice: " + totalPrice.ToStringFormatted() + "\n";
        result += "  depth: " + depth.ToString() + "\n";
        result += "  traderId: " + traderId.ToString() + "\n";

        if (preset) {
            result += "  preset: " + preset.ToStringFormatted() + "\n";
        }
        
        if (networkId && !networkId.IsEmpty()) {
            result += "  networkId: " + networkId.ToString() + "\n";
        }
        
        result += "}";
        return result;
    }
}