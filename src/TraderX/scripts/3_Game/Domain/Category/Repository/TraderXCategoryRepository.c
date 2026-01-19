class TraderXCategoryRepository
{
    private static ref map<string, ref TraderXCategory> s_Categories = new map<string, ref TraderXCategory>();
    
    static TraderXCategory GetCategoryById(string id)
    {
        return s_Categories.Get(id);
    }

    static array<ref TraderXCategory> GetCategories()
    {
        array<ref TraderXCategory> categories = new array<ref TraderXCategory>();
        foreach(string categoryId, TraderXCategory category: s_Categories)
        {
            categories.Insert(category);
        }
        return categories;
    }

    static string GetCategoryIdByName(string catName)
    {
        foreach(string categoryId, TraderXCategory category: s_Categories)
        {
            if(CF_String.EqualsIgnoreCase(category.categoryName, catName))
                return categoryId;
        }
        return string.Empty;
    }

    static void SetCategories(array<ref TraderXCategory> categories)
    {
        foreach (int i, TraderXCategory category : categories)
        {
            s_Categories.Set(category.categoryId, category);
        }
    }

    static void SaveAllCategories()
    {
        foreach (string oCategoryId, TraderXCategory oCategory : s_Categories)
        {
            Save(oCategory);
        }
    }

    static void SetAllTraderItemsProduct()
    {
        foreach(string categoryId, TraderXCategory category: s_Categories)
        {
            if(!category){
                GetTraderXLogger().LogError("one category in the array is null");
                continue;
            }

            SetTraderXProductsProduct(category);
        }
    }

    static void SetTraderXProductsProduct(TraderXCategory category)
    {
        array<ref TraderXProduct> products = category.GetProducts();
        foreach(int i, TraderXProduct product: products)
        {
            if(!product){
                continue;
            }

            TraderXProductRepository.AddItemToItems(product);
        }
    }

    static void AddCategoryToCategories(TraderXCategory category)
    {
        if(!category){
            GetTraderXLogger().LogError("one category in the array is null");
            return;
        }

        s_Categories.Set(category.categoryId, category);
    }

    static void AssignCategoriesIdsToMap()
    {
        foreach(string categoryId, TraderXCategory category: s_Categories)
        {
            if(!category){
                GetTraderXLogger().LogError("one category in the array is null");
                continue;
            }

            TraderXCategoryId.AssignIdIfNot(category);
            Save(category);
        }
    }

    static void Save(TraderXCategory category)
    {
        string filePath = string.Format(TRADERX_CATEGORY_FILE, category.categoryId);
        // Use mapper to convert to JSON format for saving
        TraderXJsonCategory jsonCategory = TraderXCategoryMapper.MapToTraderXJsonCategory(category);
        JsonFileLoader<TraderXJsonCategory>.JsonSaveFile(filePath, jsonCategory);
    }

    static TraderXCategory LoadCategory(string filePath)
    {
        if (FileExist(filePath)) {
            // Load JSON category and convert to domain object using mapper
            TraderXJsonCategory jsonCategory = new TraderXJsonCategory();
            TraderXJsonLoader<TraderXJsonCategory>.LoadFromFile(filePath, jsonCategory);
            if(jsonCategory)
            {
                GetTraderXLogger().LogDebug("[TraderX] LoadCategory " + filePath + " success");
                return TraderXCategoryMapper.MapToTraderXCategory(jsonCategory);
            }
        }
        return null;
    }

    static void DebugSaveAllCategories()
    {
        GetTraderXLogger().LogDebug("DebugSaveAllCategories");
        if(!FileExist(TRADERX_CONFIG_DIR_SERVER))
        {
            MakeDirectory(TRADERX_CONFIG_DIR_SERVER);
        }

        if (!FileExist(TRADERX_CATEGORIES_DIR)){
            GetTraderXLogger().LogDebug("[TraderX] Categories directory doesn't exist, creating: " + TRADERX_CATEGORIES_DIR);
            MakeDirectory(TRADERX_CATEGORIES_DIR);
        }
        TraderXJsonLoader<map<string, ref TraderXCategory>>.SaveToFile(TRADERX_CONFIG_DIR_SERVER + "MapAllCategories.json", s_Categories);
    }

    static void LoadAllCategories()
    {
        GetTraderXLogger().LogInfo("[TraderX] LoadAllCategories - Starting");
        
        // Try compiled JSON first (CSV-based system)
        if (FileExist(TRADERX_COMPILED_CATEGORIES_FILE))
        {
            GetTraderXLogger().LogInfo("[TraderX] Loading categories from compiled config");
            LoadFromCompiledJson();
            return;
        }
        
        // Fallback to legacy multi-file JSON
        GetTraderXLogger().LogWarning("[TraderX] Compiled config not found, using legacy multi-file JSON");
        LoadFromLegacyJson();
    }
    
    // Load categories from compiled JSON (CSV-based system)
    private static void LoadFromCompiledJson()
    {
        ref array<ref TraderXJsonCategory> jsonCategories = new array<ref TraderXJsonCategory>;
        JsonFileLoader<array<ref TraderXJsonCategory>>.JsonLoadFile(TRADERX_COMPILED_CATEGORIES_FILE, jsonCategories);
        
        if (!jsonCategories || jsonCategories.Count() == 0)
        {
            GetTraderXLogger().LogError("[TraderX] Failed to load compiled categories, falling back to legacy");
            LoadFromLegacyJson();
            return;
        }
        
        s_Categories.Clear();
        
        foreach (TraderXJsonCategory jsonCategory : jsonCategories)
        {
            TraderXCategory category = TraderXCategoryMapper.MapToTraderXCategory(jsonCategory);
            if (category && category.categoryId.Length() > 0)
            {
                s_Categories.Set(category.categoryId, category);
            }
        }
        
        GetTraderXLogger().LogInfo(string.Format("[TraderX] LoadAllCategories - Completed. Total categories loaded: %1 (from compiled config)", s_Categories.Count()));
    }
    
    // Load categories from legacy multi-file JSON (backward compatibility)
    private static void LoadFromLegacyJson()
    {
        if (!FileExist(TRADERX_CATEGORIES_DIR)){
            GetTraderXLogger().LogDebug("[TraderX] Categories directory doesn't exist, creating: " + TRADERX_CATEGORIES_DIR);
            MakeDirectory(TRADERX_CATEGORIES_DIR);
        }

        GetTraderXLogger().LogDebug("[TraderX] Clearing existing categories");
        s_Categories.Clear();

        TraderXCategory category;
        string filename;
        FileAttr attr;
        
        GetTraderXLogger().LogDebug("[TraderX] Starting file search in: " + TRADERX_CATEGORIES_DIR);
        FindFileHandle findHandle = FindFile(TRADERX_CATEGORIES_DIR + "*.json", filename, attr, FindFileFlags.ALL);
        
        GetTraderXLogger().LogDebug("[TraderX] First filename found: " + filename);
        
        if (findHandle == 0){
            GetTraderXLogger().LogDebug("[TraderX] ERROR: File handle is 0, no files found or directory error");
            CloseFindFile(findHandle);
            return;
        }

        // Process first file if it has valid UUID
        if (TraderXCategoryId.IsValidUUID(filename)) {
            category = LoadCategory(TRADERX_CATEGORIES_DIR + filename);
            if(category){
                filename.Replace(".json", "");
                TraderXCategoryId.AssignIdIfNot(category, filename);
            }
            else{
                GetTraderXLogger().LogDebug("[TraderX] Failed to load category from file: " + filename);
            }
        } else {
            GetTraderXLogger().LogDebug("[TraderX] Skipping invalid UUID filename: " + filename);
        }

        GetTraderXLogger().LogDebug("[TraderX] Starting to load remaining categories");
        while (FindNextFile(findHandle, filename, attr))
        {
            if (!TraderXCategoryId.IsValidUUID(filename)) {
                GetTraderXLogger().LogDebug("[TraderX] Skipping invalid UUID filename: " + filename);
                continue;
            }

            category = LoadCategory(TRADERX_CATEGORIES_DIR + filename);
            if(category){
                filename.Replace(".json", "");
                TraderXCategoryId.AssignIdIfNot(category, filename);
            }
        }

        CloseFindFile(findHandle);
        AssignCategoriesIdsToMap();
        GetTraderXLogger().LogInfo(string.Format("[TraderX] LoadAllCategories - Completed. Total categories loaded: %1 (from legacy JSON)", s_Categories.Count()));
    }
}