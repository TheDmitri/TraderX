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
        GetTraderXLogger().LogInfo(string.Format("[TraderX] LoadAllProducts - Completed. Total products loaded: %1", s_Items.Count()));
    }
}