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

    int tradeMode = ETraderXTradeMode.SELL;

    void TraderXTradingService()
    {
        traderCategories = new array<ref TraderXCategory>();
    }

    void SetTraderNpc(TraderXNpc npc)
    {
        GetTraderXLogger().LogDebug("SetTraderCategoriesFromNpc " + npc.npcId );
        traderNpc = npc;

        GetRPCManager().SendRPC("TraderX", "OnTraderXMenuOpen", new Param1<int>(traderNpc.npcId), true, null);
        
        traderCategories.Clear();
        foreach(UUID categoryId : npc.categoriesId)
        {
            traderCategories.Insert(TraderXCategoryRepository.GetCategoryById(categoryId));
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
        return Ternary<int>.If(traderNpc != null, traderNpc.npcId, -1);
    }

    int GetPlayerMoneyAmount(PlayerBase player)
    {
        TStringArray acceptedCurrencyTypes = traderNpc.GetCurrenciesAccepted();
        return TraderXCurrencyService.GetInstance().GetPlayerMoneyFromAllCurrency(player, acceptedCurrencyTypes);
    }

    array<ref TraderXCategory> GetTraderCategories()
    {
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
        Event_OnTraderXResponseReceived.Invoke(response, transactionResultCollection);
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