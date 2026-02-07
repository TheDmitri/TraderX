class TraderXProductRepository
{
    private static ref map<string, ref TraderXProduct> s_Items = new map<string, ref TraderXProduct>();

    static array<ref TraderXProduct> GetProducts()
    {
        array<ref TraderXProduct> products = new array<ref TraderXProduct>();
        foreach(string productId, TraderXProduct product: s_Items)
        {
            products.Insert(product);
        }
        return products;
    }

    static void SetProducts(array<ref TraderXProduct> items)
    {
        s_Items.Clear();
        foreach (int i, TraderXProduct item : items)
        {
            s_Items.Set(item.productId, item);
        }
    }

    static TraderXProduct GetItemById(string id)
    {
        return s_Items.Get(id);
    }

    static void AddItemToItems(TraderXProduct product)
    {
        s_Items.Set(product.productId, product);
    }

    static void DebugSaveAllItems()
    {
        if (!FileExist(TRADERX_PRODUCTS_DIR)){
            MakeDirectory(TRADERX_PRODUCTS_DIR);
        }
        TraderXJsonLoader<map<string, ref TraderXProduct>>.SaveToFile(TRADERX_PRODUCTS_DIR + "AllItems.json", s_Items);
    }

    static void SaveAllProducts()
    {
        foreach (string oProductId, TraderXProduct oProduct : s_Items)
        {
            Save(oProduct);
        }
    }

    static void Save(TraderXProduct product)
    {
        GetTraderXLogger().LogDebug("Save " + product.productId);
        string filePath = string.Format(TRADERX_PRODUCT_FILE, product.productId);
        JsonFileLoader<TraderXJsonProduct>.JsonSaveFile(filePath, TraderXProductMapper.MapToTraderXJsonProduct(product));
    }

    static TraderXProduct LoadProduct(string filePath)
    {
        TraderXJsonProduct jsonProduct = new TraderXJsonProduct();

        if(!FileExist(filePath)){
            GetTraderXLogger().LogWarning("[TraderX] LoadProduct " + filePath + " doesn't exist");
            return null;
        }

        TraderXJsonLoader<TraderXJsonProduct>.LoadFromFile(filePath, jsonProduct);
        if(jsonProduct) {
            GetTraderXLogger().LogDebug("[TraderX] LoadProduct " + filePath + " success");
            return TraderXProductMapper.MapToTraderXProduct(jsonProduct);
        }

        return null;	
    }

    static void LoadAllProducts()
    {
        GetTraderXLogger().LogInfo("[TraderX] LoadAllProducts - Starting");
        
        // Try compiled JSON first (CSV-based system)
        if (FileExist(TRADERX_COMPILED_PRODUCTS_FILE))
        {
            GetTraderXLogger().LogInfo("[TraderX] Loading products from compiled config");
            LoadFromCompiledJson();
            return;
        }
        
        // Fallback to legacy multi-file JSON
        GetTraderXLogger().LogWarning("[TraderX] Compiled config not found, using legacy multi-file JSON");
        LoadFromLegacyJson();
    }
    
    // Load products from compiled JSON (CSV-based system)
    private static void LoadFromCompiledJson()
    {
        ref array<ref TraderXCompiledProduct> jsonProducts = new array<ref TraderXCompiledProduct>;
        JsonFileLoader<array<ref TraderXCompiledProduct>>.JsonLoadFile(TRADERX_COMPILED_PRODUCTS_FILE, jsonProducts);
        
        if (!jsonProducts || jsonProducts.Count() == 0)
        {
            GetTraderXLogger().LogError("[TraderX] Failed to load compiled products (empty or null), falling back to legacy");
            LoadFromLegacyJson();
            return;
        }
        
        s_Items.Clear();
        int successCount = 0;
        int failCount = 0;
        
        foreach (TraderXCompiledProduct jsonProduct : jsonProducts)
        {
            if (!jsonProduct)
            {
                failCount++;
                continue;
            }
            
            // Validate required fields
            if (jsonProduct.productId.Length() == 0)
            {
                GetTraderXLogger().LogWarning("[TraderX] Skipping product with empty productId in compiled file");
                failCount++;
                continue;
            }
            
            TraderXProduct product = TraderXProductMapper.MapToTraderXProduct(jsonProduct);
            if (product)
            {
                product.productId = jsonProduct.productId;
                if (product.productId.Length() > 0)
                {
                    s_Items.Set(product.productId, product);
                    successCount++;
                }
                else
                {
                    failCount++;
                }
            }
            else
            {
                failCount++;
            }
        }
        
        // If too many failures, fall back to legacy
        if (successCount == 0 && failCount > 0)
        {
            GetTraderXLogger().LogError(string.Format("[TraderX] All %1 products failed to load from compiled file, falling back to legacy", failCount));
            LoadFromLegacyJson();
            return;
        }
        
        if (failCount > 0)
        {
            GetTraderXLogger().LogWarning(string.Format("[TraderX] Loaded %1 products from compiled file, %2 failed", successCount, failCount));
        }
        else
        {
            GetTraderXLogger().LogInfo(string.Format("[TraderX] LoadAllProducts - Completed. Total products loaded: %1 (from compiled config)", s_Items.Count()));
        }
    }
    
    // Load products from legacy multi-file JSON (backward compatibility)
    private static void LoadFromLegacyJson()
    {
        if (!FileExist(TRADERX_PRODUCTS_DIR)){
            MakeDirectory(TRADERX_PRODUCTS_DIR);
        }

        s_Items.Clear();

        TraderXProduct tProduct;
        string filename;
        FileAttr attr;
        
        GetTraderXLogger().LogDebug("[TraderX] Starting file search in: " + TRADERX_PRODUCTS_DIR);
        FindFileHandle findHandle = FindFile(TRADERX_PRODUCTS_DIR + "*.json", filename, attr, FindFileFlags.ALL);
        
        GetTraderXLogger().LogDebug("[TraderX] First filename found: " + filename);
        
        if (findHandle == 0){
            GetTraderXLogger().LogError("[TraderX] ERROR: File handle is 0, no files found or directory error");
            CloseFindFile(findHandle);
            return;
        }

        // Process first file if it has valid UUID
        if (TraderXProductId.IsValidUUID(filename)) {
            tProduct = LoadProduct(TRADERX_PRODUCTS_DIR + filename);
            if(tProduct){
                GetTraderXLogger().LogDebug("SET : " + filename + " in s_items with id " + tProduct.productId);
                filename.Replace(".json", "");
                TraderXProductId.AssignUUIDIfNot(tProduct, filename);
                s_Items.Set(filename, tProduct);
            } else {
                GetTraderXLogger().LogError("[TraderX] Failed to load first product file");
            }
        } else {
            GetTraderXLogger().LogWarning("[TraderX] Skipping invalid UUID filename: " + filename);
        }

        GetTraderXLogger().LogDebug("[TraderX] Starting to load remaining products");
        while (FindNextFile(findHandle, filename, attr))
        {
            if (!TraderXProductId.IsValidUUID(filename)) {
                GetTraderXLogger().LogWarning("[TraderX] Skipping invalid UUID filename: " + filename);
                continue;
            }
            
            tProduct = LoadProduct(TRADERX_PRODUCTS_DIR + filename);
            if(tProduct){
                filename.Replace(".json", "");
                TraderXProductId.AssignUUIDIfNot(tProduct, filename);
                GetTraderXLogger().LogDebug("SET : " + filename + " in s_items with id " + tProduct.productId);
                s_Items.Set(filename, tProduct);
            } else {
                GetTraderXLogger().LogError("[TraderX] Failed to load product from file: " + filename);
            }
        }

        CloseFindFile(findHandle);
        GetTraderXLogger().LogInfo(string.Format("[TraderX] LoadAllProducts - Completed. Total products loaded: %1 (from legacy JSON)", s_Items.Count()));
    }
}