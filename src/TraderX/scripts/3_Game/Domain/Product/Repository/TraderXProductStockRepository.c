class TraderXProductStockRepository
{
    static ref map<string, ref TraderXProductStock> s_itemsStockByItemId = new map<string, ref TraderXProductStock>();

    static void GetStockArrForCategoriesId(array<string> categoriesId, out array<ref TraderXProductStock> stockArr)
    {
        // Pre-load all stock for categories to avoid N+1 file I/O problem
        LoadStockForCategories(categoriesId);
        
        foreach(string categoryId: categoriesId)
        {
            TraderXCategory category = TraderXCategoryRepository.GetCategoryById(categoryId);
            if(!category)
                continue;

            foreach(TraderXProduct tpItem: category.GetProducts())
            { 
                // Check for null product to prevent crash
                if (!tpItem) {
                    GetTraderXLogger().LogWarning("GetStockArrForCategoriesId: Found null product in category " + categoryId);
                    continue;
                }
                
                // Stock should already be loaded by LoadStockForCategories
                TraderXProductStock stock = s_itemsStockByItemId[tpItem.productId];
                if (stock) {
                    stockArr.Insert(stock);
                }
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
        
        // Collect all product IDs that need stock loading
        array<string> productIdsToLoad = new array<string>();
        
        foreach(string categoryId: categoriesId)
        {
            TraderXCategory category = TraderXCategoryRepository.GetCategoryById(categoryId);
            if(!category)
                continue;
                
            foreach(TraderXProduct product: category.GetProducts())
            {
                if (!product || product.IsStockUnlimited())
                    continue;
                    
                // Only load if not already in memory
                if (!s_itemsStockByItemId.Contains(product.productId)) {
                    productIdsToLoad.Insert(product.productId);
                }
            }
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
                // Create new stock file for products that don't have one
                s_itemsStockByItemId.Set(productId, itemStock);
                Save(itemStock);
            }
        }
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
    
    static bool CanIncreaseStock(string productId, int maxStock)
    {
        if (maxStock == -1) return true; // Unlimited stock
        return GetStockAmount(productId) < maxStock;
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
