/*
=============================================================================
     Program: TraderX Trading Service
     File: TraderXTradingService.c
     Author: Dmitri
     Version: v1.0
     Date: 07/02/2024

     Description: This program is designed to manage all trading activities in 
                  the TraderX Menu. The system keeps track of a list of 
                  tradeable categories, and provides a way to set this list from a 
                  TraderXNpc object. 
=============================================================================
*/
class TraderXTradingService
{
    ref array<ref TraderXCategory> traderCategories;
    ref TraderXNpc traderNpc;
    static ref TraderXTradingService m_instanceTraderXTradingService;
    static ref ScriptInvoker Event_OnTraderXResponseReceived = new ScriptInvoker();
    private bool isMaxQuantity = false;
    private bool m_IsTransactionPending = false;
    // True once the opened trader's products have fully streamed in (products are sent before
    // stock, so ALL_STOCK_RECEIVED marks completion). Reset each time a trader menu opens.
    // Pages read this on mount so a tab opened AFTER completion isn't stuck showing "syncing".
    private bool m_ProductsReady = false;

    int tradeMode = ETraderXTradeMode.SELL;

    void TraderXTradingService()
    {
        traderCategories = new array<ref TraderXCategory>();
    }

    void SetTraderNpc(TraderXNpc npc)
    {
        GetTraderXLogger().LogDebug("SetTraderCategoriesFromNpc " + npc.npcId );
        traderNpc = npc;

        // If the client already holds this trader's products (prewarmed on safezone entry), skip
        // the product stream: mark ready now (so pages show expand-all with no sync flash) and
        // tell the server we only need stock. Otherwise the server streams products then stock.
        bool hasProducts = HasNpcProducts(npc);
        m_ProductsReady = hasProducts;
        int catCount = 0;
        if (npc.categoriesId) catCount = npc.categoriesId.Count();
        GetTraderXLogger().LogDebug("SetTraderNpc npc=" + npc.npcId + " categoriesId=" + catCount + " hasProducts=" + hasProducts);
        GetRPCManager().SendRPC("TraderX", "OnTraderXMenuOpen", new Param2<int, bool>(traderNpc.npcId, !hasProducts), true, null);
        
        traderCategories.Clear();
        foreach(string categoryId : npc.categoriesId)
        {
            TraderXCategory category = TraderXCategoryRepository.GetCategoryById(categoryId);
            if (category)
                traderCategories.Insert(category);
        }
    }

    void SetTradeMode(int mode)
    {
        tradeMode = mode;
    }

    int GetTradeMode()
    {
        return tradeMode;
    }

    bool IsMaxQuantity()
    {
        return isMaxQuantity;
    }

    void SetMaxQuantity(bool value)
    {
        isMaxQuantity = value;
    }

    void ClearNpc()
    {
        if(traderNpc){
            GetRPCManager().SendRPC("TraderX", "OnTraderXMenuClose", new Param1<int>(traderNpc.npcId), true, null);
            traderNpc = null;
        }
        
        traderCategories.Clear();
    }

    int GetNpcId()
    {
        // No ternary helper here: its arguments are evaluated eagerly, so traderNpc.npcId
        // would dereference null after the menu closed.
        if (!traderNpc)
            return -1;
        return traderNpc.npcId;
    }

    int GetPlayerMoneyAmount(PlayerBase player)
    {
        // traderNpc is cleared by OnTraderXMenuClose before the menu components are destroyed,
        // and teardown (DeSelectAllItems -> checkout refresh) still asks for the wallet.
        if (!traderNpc || !player)
            return 0;
        TStringArray acceptedCurrencyTypes = traderNpc.GetCurrenciesAccepted();
        return TraderXCurrencyService.GetInstance().GetPlayerMoneyFromAllCurrency(player, acceptedCurrencyTypes);
    }

    // Rebuilt fresh in the trader's assigned order (npc.categoriesId) every call, skipping
    // any not-yet-resolved category. This guarantees the menu renders categories in exactly
    // the order they're assigned to this trader — and stays correct even if some categories
    // resolved after SetTraderNpc ran (which would otherwise leave null gaps).
    array<ref TraderXCategory> GetTraderCategories()
    {
        traderCategories.Clear();
        if (traderNpc)
        {
            foreach (string categoryId : traderNpc.categoriesId)
            {
                TraderXCategory category = TraderXCategoryRepository.GetCategoryById(categoryId);
                if (category)
                    traderCategories.Insert(category);
            }
        }
        return traderCategories;
    }

    static TraderXTradingService GetInstance()
    {
        if (!m_instanceTraderXTradingService)
        {
            m_instanceTraderXTradingService = new TraderXTradingService();
        }
        return m_instanceTraderXTradingService;
    }

    bool IsTransactionPending()
    {
        return m_IsTransactionPending;
    }

    void LockTransaction()
    {
        m_IsTransactionPending = true;
    }

    void UnlockTransaction()
    {
        m_IsTransactionPending = false;
    }

    void OnTraderXResponseReceived(int response, TraderXTransactionResultCollection transactionResultCollection = null)
    {
        GetTraderXLogger().LogDebug("TraderXTradingService::OnTraderXResponseReceived");
        if(response == ETraderXResponse.TRANSACTIONS)
            UnlockTransaction();
        // Stock is sent after the last product chunk, so this marks the trader's catalog ready.
        if(response == ETraderXResponse.ALL_STOCK_RECEIVED)
            m_ProductsReady = true;
        Event_OnTraderXResponseReceived.Invoke(response, transactionResultCollection);
    }

    // True once the opened trader's products have fully arrived (see m_ProductsReady).
    bool IsProductsReady()
    {
        return m_ProductsReady;
    }

    // Client-side: does the product repo already hold every product of this trader's categories?
    // Used to decide whether the server must stream products on open (false) or just stock (true).
    // Checks category productIds only — if those are present the trader's prewarm ran, which also
    // pulled their variants/attachments.
    bool HasNpcProducts(TraderXNpc npc)
    {
        if (!npc)
            return false;

        foreach (string categoryId : npc.categoriesId)
        {
            TraderXCategory category = TraderXCategoryRepository.GetCategoryById(categoryId);
            if (!category)
            {
                GetTraderXLogger().LogDebug("HasNpcProducts: category not found in client repo: " + categoryId);
                return false;
            }

            int pidCount = 0;
            if (category.productIds) pidCount = category.productIds.Count();
            int resolved = 0;
            foreach (string productId : category.productIds)
            {
                TraderXProduct listedProduct = TraderXProductRepository.GetItemById(productId);
                if (listedProduct && HasLinkedProducts(listedProduct))
                    resolved++;
                else
                {
                    GetTraderXLogger().LogDebug("HasNpcProducts: missing product (or its variants/attachments) in client repo: " + productId + " (cat " + categoryId + ", productIds=" + pidCount + ", resolved " + resolved + ")");
                    return false;
                }
            }
            GetTraderXLogger().LogDebug("HasNpcProducts: cat " + categoryId + " productIds=" + pidCount + " all resolved");
        }
        return true;
    }

    // A listed product may already be cached because ANOTHER trader streamed it as a variant or
    // attachment of one of its own products — that stream did not include this product's own
    // variants/attachments. Require them too, otherwise the server skips the stream and the
    // Customize page shows no variants/attachments.
    bool HasLinkedProducts(TraderXProduct product)
    {
        if (product.variants)
        {
            foreach (string variantId : product.variants)
            {
                if (variantId != "" && !TraderXProductRepository.GetItemById(variantId))
                    return false;
            }
        }
        if (product.attachments)
        {
            foreach (string attachmentId : product.attachments)
            {
                if (attachmentId != "" && !TraderXProductRepository.GetItemById(attachmentId))
                    return false;
            }
        }
        return true;
    }

    void GetFavoritesCategories(out array<ref TraderXCategory> categories)
    {
        categories.Insert(TraderXCategory.CreateCategory("#tpm_favorites"));
        foreach(TraderXCategory traderXCategory : traderCategories)
        {
            if(!traderXCategory)
                continue;
            
            array<ref TraderXProduct> products = traderXCategory.GetProducts();
            foreach(TraderXProduct item : products)
            {
                if(!item.CanBeBought())
                    continue;
                
                if(TraderXFavoritesService.GetInstance().IsFavorite(item))
                    categories[0].productIds.Insert(item.productId);
            }
        }
    }

    void GetTraderXProductsFromSlotId(PlayerBase player, int slotId, out array<ref TraderXProduct> items)
    {
        GetTraderXLogger().LogDebug("GetTraderXProductsFromSlotId slotId: " + slotId);
        array<EntityAI> playerItems = new array<EntityAI>();
        TraderXInventoryManager.GetPlayerEntitiesFromSlotId(player, slotId, playerItems);

        foreach(TraderXCategory category : traderCategories)
        {
            GetTraderXLogger().LogDebug("GetTraderXProductsFromSlotId category: " + category.categoryName);
            foreach(EntityAI playerItem : playerItems)
            {
                if(!playerItem)
                    continue;

                GetTraderXLogger().LogDebug("GetTraderXProductsFromSlotId playerItem: " + playerItem.GetType());
                TraderXProduct traderXItem = category.FindProductByClassName(playerItem.GetType());
                if(!traderXItem)
                    continue;

                if(!traderXItem.CanBeSold())
                    continue;

                int depth = playerItem.GetHierarchyLevel();

                int highId = -1;
                int lowId = -1;
                playerItem.GetNetworkID(lowId, highId);

                GetTraderXLogger().LogDebug("item: " + playerItem.GetType() + " highId: " + highId + " lowId: " + lowId);
                items.Insert(TraderXProduct.CreateAsPlayerItem(playerItem.GetType(), highId, lowId, depth, traderXItem, playerItem.GetHealthLevel()));
                GetTraderXLogger().LogDebug("items count: " + items.Count());	
            }
        }
    }
}