class TraderXProductStockRepository
{
    static ref map<string, ref TraderXProductStock> s_itemsStockByItemId = new map<string, ref TraderXProductStock>();

    // Every productId a player can transact with at these NPCs: the category products
    // PLUS each product's variants and preset attachments. Variants/attachments are not
    // members of any category, so without including them here their stock never loads on
    // the server's "for categories" path nor syncs to the client. The client then prices
    // them at stock 0 while the server uses the real stock, and the buy is rejected by the
    // server's price-match (anti-tamper) check — variants appear unbuyable. Deduplicated.
    static void CollectTradableProductIds(array<string> categoriesId, out array<string> outIds)
    {
        foreach(string categoryId: categoriesId)
        {
            TraderXCategory category = TraderXCategoryRepository.GetCategoryById(categoryId);
            if(!category)
                continue;

            foreach(TraderXProduct product: category.GetProducts())
            {
                if (!product)
                {
                    GetTraderXLogger().LogWarning("CollectTradableProductIds: Found null product in category " + categoryId);
                    continue;
                }

                if (outIds.Find(product.productId) == -1)
                    outIds.Insert(product.productId);

                if (product.variants)
                {
                    foreach(string variantId: product.variants)
                    {
                        if (variantId == "")
                            continue;
                        if (outIds.Find(variantId) == -1)
                            outIds.Insert(variantId);
                        // A variant has its own attachment list (shown in Customize once the
                        // variant is picked) — stream those too, or they are missing client-side.
                        TraderXProduct variantProduct = TraderXProductRepository.GetItemById(variantId);
                        if (variantProduct)
                            CollectAttachmentIds(variantProduct, outIds);
                    }
                }

                CollectAttachmentIds(product, outIds);
            }
        }
    }

    static void CollectAttachmentIds(TraderXProduct product, array<string> outIds)
    {
        if (!product || !product.attachments)
            return;

        foreach(string attachmentId: product.attachments)
        {
            if (attachmentId != "" && outIds.Find(attachmentId) == -1)
                outIds.Insert(attachmentId);
        }
    }

    static void GetStockArrForCategoriesId(array<string> categoriesId, out array<ref TraderXProductStock> stockArr)
    {
        // Pre-load all stock for categories to avoid N+1 file I/O problem
        LoadStockForCategories(categoriesId);

        array<string> productIds = new array<string>();
        CollectTradableProductIds(categoriesId, productIds);

        foreach(string productId: productIds)
        {
            // Stock should already be loaded by LoadStockForCategories
            TraderXProductStock stock = s_itemsStockByItemId[productId];
            if (stock) {
                stockArr.Insert(stock);
            }
        }
    }

    static void RefStockToTraderXProduct(array<ref TraderXProductStock> stockArr)
    {
        GetTraderXLogger().LogDebug("RefStockToTraderXProduct - Loading " + stockArr.Count() + " stock entries into client cache");
        foreach(TraderXProductStock itemStock: stockArr)
        {
            if(!itemStock)
                continue;
            
            // Store in client-side cache (same pattern as server)
            s_itemsStockByItemId.Set(itemStock.productId, itemStock);
            GetTraderXLogger().LogDebug("RefStockToTraderXProduct: Cached stock for " + itemStock.productId + " (stock: " + itemStock.GetStock() + ")");
        }
    }
    
    // Client-side stock access methods (mirrors server API)
    static string GetStockDisplay(string productId, int maxStock)
    {
        if (maxStock == -1) return "unlimited";
        
        TraderXProductStock stock = s_itemsStockByItemId[productId];
        if (!stock) return "0/" + maxStock.ToString();
        
        return stock.GetStock().ToString() + "/" + maxStock.ToString();
    }
    
    static int GetStockValue(string productId)
    {
        TraderXProductStock stock = s_itemsStockByItemId[productId];
        if(!stock)
            return 0;
        
        return stock.GetStock();
    }
    
    static bool IsStockReached(string productId, int maxStock)
    {
        if (maxStock == -1) return false;
        
        TraderXProductStock stock = s_itemsStockByItemId[productId];
        if(!stock)
            return false;
        
        return stock.GetStock() >= maxStock;
    }

    static void LoadStockForCategories(array<string> categoriesId)
    {
        GetTraderXLogger().LogInfo("LoadStockForCategories: Bulk loading stock for " + categoriesId.Count() + " categories");
        
        // Collect all tradable product IDs (category products + their variants + attachments)
        array<string> tradableIds = new array<string>();
        CollectTradableProductIds(categoriesId, tradableIds);

        // Keep only the finite-stock ones not already in memory.
        array<string> productIdsToLoad = new array<string>();
        foreach(string productId: tradableIds)
        {
            TraderXProduct product = TraderXProductRepository.GetItemById(productId);
            if (!product || product.IsStockUnlimited())
                continue;

            if (!s_itemsStockByItemId.Contains(productId))
                productIdsToLoad.Insert(productId);
        }

        // Bulk load all required stock files
        LoadStockBulk(productIdsToLoad);
        
        
        GetTraderXLogger().LogInfo("LoadStockForCategories: Loaded " + productIdsToLoad.Count() + " stock files");
    }
    
    static void LoadStockBulk(array<string> productIds)
    {
        MakeDirectoryIfNotExists();
        
        foreach(string productId: productIds)
        {
            string filePath = string.Format(TRADERX_STOCK_FILE, productId);
            TraderXProductStock itemStock = new TraderXProductStock(productId, 0);
            
            if (FileExist(filePath)) {
                string errorMessage;
                if (JsonFileLoader<TraderXProductStock>.LoadFile(filePath, itemStock, errorMessage)) {
                    s_itemsStockByItemId.Set(productId, itemStock);
                } else {
                    GetTraderXLogger().LogError("LoadStockBulk: Failed to load " + filePath + " - " + errorMessage);
                    s_itemsStockByItemId.Set(productId, itemStock); // Use default stock
                }
            } else {
                // Create new stock file for products that don't have one: a limited product starts full
                itemStock.SetStock(GetInitialStock(productId));
                s_itemsStockByItemId.Set(productId, itemStock);
                Save(itemStock);
            }
        }
    }

    // First-time stock for a product with no stock file: maxStock for limited products, 0 otherwise.
    static int GetInitialStock(string productId)
    {
        TraderXProduct product = TraderXProductRepository.GetItemById(productId);
        if (product && product.maxStock > 0)
            return product.maxStock;
        return 0;
    }

    static void SetStockMax(string productId, int maxStock)
    {
        TraderXProductStock stock = GetStockByProductId(productId);
        if (stock) {
            stock.SetStock(maxStock);
            Save(stock);
        }
    }
    
    static void SetRandomStock(string productId, int maxStock)
    {
        TraderXProductStock stock = GetStockByProductId(productId);
        if (stock) {
            stock.SetStock(Math.RandomIntInclusive(0, maxStock));
            Save(stock);
        }
    }
    
    static void DeStock(string productId, float deStockCoefficient)
    {
        if (deStockCoefficient <= 0) return;
        
        TraderXProductStock stock = GetStockByProductId(productId);
        if (!stock) return;
        
        int currentStock = stock.GetStock();
        int newStock = Math.Round(currentStock * (1.0 - deStockCoefficient));
        stock.SetStock(newStock);
        Save(stock);
    }
    
    static void LoadCategoryStock(string productId, int stockBehavior, int maxStock, float deStockCoefficient)
    {
        if (stockBehavior == 1) {
            SetStockMax(productId, maxStock);
        } else if (stockBehavior == 2) {
            SetRandomStock(productId, maxStock);
        }
        
        if (deStockCoefficient > 0) {
            DeStock(productId, deStockCoefficient);
        }
    }

    static void MakeDirectoryIfNotExists()
    {
        if (!FileExist(TRADERX_DB_DIR_SERVER)){
            MakeDirectory(TRADERX_DB_DIR_SERVER);
        }

        if (!FileExist(TRADERX_STOCK_DIR)){
            MakeDirectory(TRADERX_STOCK_DIR);
        }
    }

    static TraderXProductStock GetStockByProductId(string productId)
    {
        // Check cache first
        if (s_itemsStockByItemId.Contains(productId)) {
            return s_itemsStockByItemId[productId];
        }
        
        // Load if not in cache
        return LoadItemStockFromId(productId);
    }
    
    static bool HasStock(string productId, int requiredAmount = 1)
    {
        TraderXProductStock stock = GetStockByProductId(productId);
        return stock && stock.GetStock() >= requiredAmount;
    }
    
    static int GetStockAmount(string productId)
    {
        // Check if product has unlimited stock first
        TraderXProduct product = TraderXProductRepository.GetItemById(productId);
        if (product && product.IsStockUnlimited()) {
            return -1; // Return -1 to indicate unlimited stock
        }
        
        TraderXProductStock stock = GetStockByProductId(productId);
        if(!stock)
            return 0;
        
        return stock.GetStock();
    }
    
    static bool CanDecreaseStock(string productId, int amount = 1)
    {
        return GetStockAmount(productId) >= amount;
    }
    
    static bool CanIncreaseStock(string productId, int maxStock, int amount = 1)
    {
        if (maxStock == -1) return true; // Unlimited stock
        return (GetStockAmount(productId) + Math.Max(1, amount)) <= maxStock;
    }
    
    static bool DecreaseStock(string productId, int amount = 1)
    {
        TraderXProductStock stock = GetStockByProductId(productId);
        if (!stock || stock.GetStock() < amount) {
            return false;
        }
        
        for (int i = 0; i < amount; i++) {
            stock.DecreaseStock();
        }
        Save(stock);
        return true;
    }
    
    static bool IncreaseStock(string productId, int amount = 1)
    {
        TraderXProductStock stock = GetStockByProductId(productId);
        if (!stock) {
            return false;
        }
        
        stock.IncreaseStock(amount);
        Save(stock);
        return true;
    }

    static TraderXProductStock LoadItemStockFromId(string id)
    {
        MakeDirectoryIfNotExists();

        // Check if already loaded in memory
        if (s_itemsStockByItemId.Contains(id)) {
            return s_itemsStockByItemId[id];
        }
        
        string filePath = string.Format(TRADERX_STOCK_FILE, id);
        TraderXProductStock itemStock = new TraderXProductStock(id, 0);

		if (FileExist(filePath)) {
			GetTraderXLogger().LogDebug("itemStock " + filePath + " file is loading...");
			
			// Add error handling for file loading
			string errorMessage;
			if (JsonFileLoader<TraderXProductStock>.LoadFile(filePath, itemStock, errorMessage)) {
                s_itemsStockByItemId.Set(id, itemStock);
                GetTraderXLogger().LogDebug("Successfully loaded stock for product: " + id);
			} else {
                GetTraderXLogger().LogError("Failed to load stock file: " + filePath + " - " + errorMessage + ", using default stock");
                s_itemsStockByItemId.Set(id, itemStock);
            }
			return itemStock;
		}

        GetTraderXLogger().LogDebug("Stock file " + filePath + " doesn't exist, creating new stock");
        itemStock.SetStock(GetInitialStock(id));
        s_itemsStockByItemId.Set(id, itemStock);
        Save(itemStock);
		return itemStock;
    }

    static void Save(TraderXProductStock itemStock)
    {
        string filePath = string.Format(TRADERX_STOCK_FILE, itemStock.productId);
        string errorMessage;
        if (!JsonFileLoader<TraderXProductStock>.SaveFile(filePath, itemStock, errorMessage)) {
            GetTraderXLogger().LogError("Failed to save stock file: " + filePath + " - " + errorMessage);
        }
    }
}
