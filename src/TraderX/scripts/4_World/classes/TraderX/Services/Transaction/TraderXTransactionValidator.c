class TraderXTransactionValidator
{
    bool ValidateTransaction(TraderXTransaction transaction, PlayerBase player, out string errorMessage)
    {
        errorMessage = "";
        
        if (!transaction)
        {
            errorMessage = "Transaction is null";
            return false;
        }
        
        if (!transaction.IsValid())
        {
            errorMessage = "Transaction is invalid";
            return false;
        }
        
        // Validation spécifique selon le type
        if (transaction.IsBuy())
        {
            return ValidateBuyTransaction(transaction, player, errorMessage);
        }
        else if (transaction.IsSell())
        {
            return ValidateSellTransaction(transaction, player, errorMessage);
        }
        
        return true;
    }
    
    private bool ValidateBuyTransaction(TraderXTransaction transaction, PlayerBase player, out string errorMessage)
    {
        errorMessage = "";
        
        // product validation
        TraderXProduct product = TraderXProductRepository.GetItemById(transaction.GetProductId());
        if (!product)
        {
            errorMessage = "Product not found: " + transaction.GetProductId();
            GetTraderXLogger().LogDebug("ValidateBuyTransaction : " + errorMessage);
            return false;
        }

        // stock validation
        if(!product.IsStockUnlimited() && !TraderXProductStockRepository.HasStock(transaction.GetProductId())){
            errorMessage = "Insufficient stock for: " + product.className;
            GetTraderXLogger().LogDebug("ValidateBuyTransaction : " + errorMessage);
            return false;
        }

        // npc validation
        TraderXNpc npc = TraderXNpcService.GetInstance().GetNpcById(transaction.GetTraderId());
        if(!npc){
            errorMessage = "Npc wasn't found";
            GetTraderXLogger().LogDebug("ValidateBuyTransaction : " + errorMessage);
            return false;
        }

        // money validation - skip if price is zero (free items)
        int totalPrice = transaction.GetTotalPrice().GetAmount();
        if(totalPrice > 0 && TraderXCurrencyService.GetInstance().GetPlayerMoneyFromAllCurrency(player, npc.GetCurrenciesAccepted()) < totalPrice){
            errorMessage = "Not enough money";
            GetTraderXLogger().LogDebug("ValidateBuyTransaction : " + errorMessage);
            return false;
        }
        
        return true;
    }
    
    private bool ValidateSellTransaction(TraderXTransaction transaction, PlayerBase player, out string errorMessage)
    {
        errorMessage = "";
        
        // product validation
        TraderXProduct product = TraderXProductRepository.GetItemById(transaction.GetProductId());
        if (!product)
        {
            errorMessage = "Product not found: " + transaction.GetProductId();
            GetTraderXLogger().LogDebug("ValidateSellTransaction : " + errorMessage);
            return false;
        }

        if(transaction.IsSell() && !product.CanBeSold()){
            errorMessage = "Product cannot be sold: " + product.className;
            GetTraderXLogger().LogDebug("ValidateSellTransaction : " + errorMessage);
            return false;
        }

        if(!product.IsStockUnlimited() && !TraderXProductStockRepository.CanIncreaseStock(transaction.GetProductId(), product.maxStock)){
            errorMessage = "The stock of this item is full";
            GetTraderXLogger().LogDebug("ValidateSellTransaction : " + errorMessage);
            return false;
        }
        
        // item possession validation
        if (!PlayerHasItem(player, transaction))
        {
            errorMessage = "Player does not have required item: " + product.className;
            GetTraderXLogger().LogDebug("ValidateSellTransaction : " + errorMessage);
            return false;
        }
        
        return true;
    }
    
    private bool PlayerHasItem(PlayerBase player, TraderXTransaction transaction)
    {
        if (!transaction.HasNetworkId())
            return false;
        
        EntityAI item = EntityAI.Cast(GetGame().GetObjectByNetworkId(transaction.GetNetworkId().GetLowId(), transaction.GetNetworkId().GetHighId()));
        
        return item != null;
    }
}