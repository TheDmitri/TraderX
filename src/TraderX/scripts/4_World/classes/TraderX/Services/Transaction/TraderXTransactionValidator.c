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

        // catalog validation - only products listed in this trader's categories can be bought on their own
        // (preset-only parts, e.g. imported vehicle parts priced 0, are bought through their vehicle's preset)
        if(!IsSoldByTrader(npc, transaction.GetProductId())){
            errorMessage = "Product not sold by this trader: " + transaction.GetProductId();
            GetTraderXLogger().LogWarning("ValidateBuyTransaction : " + errorMessage);
            return false;
        }

        // trader license validation
        if(!ValidateTraderLicenses(npc, player, errorMessage)){
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

        // trader license validation
        TraderXNpc npc = TraderXNpcService.GetInstance().GetNpcById(transaction.GetTraderId());
        if(!npc){
            errorMessage = "Npc wasn't found";
            GetTraderXLogger().LogDebug("ValidateSellTransaction : " + errorMessage);
            return false;
        }

        if(!ValidateTraderLicenses(npc, player, errorMessage)){
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
    
    // Server-side trader gate: the player must own every license the trader requires.
    // Client hides the trade action too, but this is the authoritative check.
    private bool ValidateTraderLicenses(TraderXNpc npc, PlayerBase player, out string errorMessage)
    {
        if(!npc.GetLicensesRequired() || npc.GetLicensesRequired().Count() == 0)
            return true;

        string playerId = "";
        if(player && player.GetIdentity())
            playerId = player.GetIdentity().GetPlainId();

        if(!TraderXLicenseService.GetInstance().HasLicensesForPlayer(playerId, npc.GetLicensesRequired()))
        {
            errorMessage = "Missing required trader license";
            GetTraderXLogger().LogDebug("ValidateTraderLicenses : " + errorMessage);
            return false;
        }

        return true;
    }

    private bool IsSoldByTrader(TraderXNpc npc, string productId)
    {
        array<string> traderCategories = npc.GetCategories();
        if(!traderCategories)
            return false;

        foreach (string categoryId : traderCategories)
        {
            TraderXCategory category = TraderXCategoryRepository.GetCategoryById(categoryId);
            if (category && category.ContainsProduct(productId))
                return true;
        }

        // Variants and attachments shown on a listed product's card are buyable too, unless free:
        // a 0-priced attachment only exists to be bundled in its parent's preset.
        TraderXProduct target = TraderXProductRepository.GetItemById(productId);
        if (!target || target.buyPrice <= 0)
            return false;

        foreach (string catId : traderCategories)
        {
            TraderXCategory cat = TraderXCategoryRepository.GetCategoryById(catId);
            if (!cat || !cat.productIds)
                continue;
            foreach (string listedId : cat.productIds)
            {
                TraderXProduct listed = TraderXProductRepository.GetItemById(listedId);
                if (!listed)
                    continue;
                if (listed.variants && listed.variants.Find(productId) != -1)
                    return true;
                if (listed.attachments && listed.attachments.Find(productId) != -1)
                    return true;
            }
        }
        return false;
    }

    private bool PlayerHasItem(PlayerBase player, TraderXTransaction transaction)
    {
        if (!transaction.HasNetworkId())
            return false;
        
        EntityAI item = EntityAI.Cast(GetGame().GetObjectByNetworkId(transaction.GetNetworkId().GetLowId(), transaction.GetNetworkId().GetHighId()));
        
        return item != null;
    }
}