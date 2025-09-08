class oldTraderXCategory
{
	string CategoryName;
	ref array<string> Products;

    TStringArray GetStringArrayFromString(string product)
    {
        TStringArray arr = new TStringArray;
        product.Split(",", arr);
        return arr;
    }

    string GetClassNameFromProduct(string product)
    {
        TStringArray arr = GetStringArrayFromString(product);
        if(arr.Count() < 1)
            return string.Empty;

        return arr[0];
    }

    float GetCoefficientFromProduct(string product)
    {
        TStringArray arr = GetStringArrayFromString(product);
        if(arr.Count() < 2)
            return 1.0;

        return arr[1].ToFloat();
    }

    int GetMaxStockFromProduct(string product)
    {
       TStringArray arr = GetStringArrayFromString(product);
        if(arr.Count() < 3)
            return -1;

        return arr[2].ToFloat();
    }

    int GetTradeQuantityFromProduct(string product)
    {
        TStringArray arr = GetStringArrayFromString(product);
        if(arr.Count() < 4)
            return -1;

        int oldTradeQuantity = arr[3].ToInt();
        float oldCoefficientQuantity = arr[3].ToFloat();
        int sellMode;
        int buyMode;
        int buyQuantity;
        int sellQuantity;

        if(oldTradeQuantity == -1){
            buyMode = 0x2;
            sellMode = 0x2;
            buyQuantity = 0;
            sellQuantity = 0;
        } else if(oldTradeQuantity == 0){
            buyMode = 0x0;
            sellMode = 0x1;
            buyQuantity = 0;
            sellQuantity = 0;
        } else if(oldCoefficientQuantity < 1.0 && oldCoefficientQuantity != 0.0 && oldCoefficientQuantity != -1.0){
            buyMode =  0x3;
            sellMode = 0x3;
            buyQuantity = oldCoefficientQuantity*100;
            sellQuantity = oldCoefficientQuantity*100;
        } else {
            buyMode = 0x4;
            sellMode = 0x4;
            buyQuantity = oldTradeQuantity;
            sellQuantity = oldTradeQuantity;
        }

        return CreateTradeQuantity(buyMode, sellMode, buyQuantity, sellQuantity);
    }

    int CreateTradeQuantity(int buyMode, int sellMode, int buyQuantity, int sellQuantity)
    {
        return sellMode | (buyMode << 3) | ((sellQuantity << 6) & 0x1FFF) | (buyQuantity << 19);
    }

    int GetBuyPriceFromProduct(string product)
    {
        TStringArray arr = GetStringArrayFromString(product);
        if(arr.Count() < 5)
            return -1;

        return arr[4].ToInt();
    }

    int GetSellPriceFromProduct(string product)
    {
        TStringArray arr = GetStringArrayFromString(product);
        if(arr.Count() < 6)
            return -1;
        
        return arr[5].ToInt();
    }

    float GetDeStockCoefficientFromProduct(string product)
    {
        TStringArray arr = GetStringArrayFromString(product);
        if(arr.Count() < 7)
            return 0.0;
        
        return arr[6].ToFloat();
    }

    TraderXProduct GetTraderXProductFromProduct(string product)
    {
        string className = GetClassNameFromProduct(product);
        float coefficient= GetCoefficientFromProduct(product);
        int maxStock = GetMaxStockFromProduct(product);
        int tradeQuantity = GetTradeQuantityFromProduct(product);
        int buyPrice = GetBuyPriceFromProduct(product);
        int sellPrice = GetSellPriceFromProduct(product);
        float deStockCoefficient = GetDeStockCoefficientFromProduct(product);

        // Generate a unique product ID based on the class name
        string productId = TraderXProductId.GenerateProductId(className);
        if (productId == string.Empty)
        {
            GetTraderXLogger().LogError(string.Format("[TraderXProduct] Failed to generate product ID for class '%1'", className));
            return null;
        }

        return TraderXProduct.CreateProduct(className, coefficient, maxStock, tradeQuantity, buyPrice, sellPrice, deStockCoefficient, 0, null, null, productId);
    }
};

class oldTraderXPriceConfig
{
    string Version = "";
	bool EnableAutoCalculation;
	bool EnableAutoDestockAtRestart;
	int EnableDefaultTraderStock;
	ref array<ref oldTraderXCategory>TraderCategories;

	void oldTraderXPriceConfig()
	{
		TraderCategories = new array<ref oldTraderXCategory>;
	}

    int GetTraderStockAtRestart()
    {
        int data;
        if(EnableAutoDestockAtRestart){
            data = data | 0x0001000;
        }

        data = data | EnableDefaultTraderStock;

        return data;
    }

    void ConvertToNewConfig()
    {
        GetTraderXLogger().LogInfo("ConvertToNewConfig");

        if (!FileExist(TRADERX_CATEGORIES_DIR)){
            GetTraderXLogger().LogDebug("[TraderX] Categories directory doesn't exist, creating: " + TRADERX_CATEGORIES_DIR);
            MakeDirectory(TRADERX_CATEGORIES_DIR);
        }

        if (!FileExist(TRADERX_PRODUCTS_DIR)){
            GetTraderXLogger().LogDebug("[TraderX] Products directory doesn't exist, creating: " + TRADERX_PRODUCTS_DIR);
            MakeDirectory(TRADERX_PRODUCTS_DIR);
        }

        foreach(oldTraderXCategory oldCategory: TraderCategories)
        {
            // Generate a unique category ID based on the category name
            string categoryId = TraderXCategoryId.GenerateCategoryId(oldCategory.CategoryName);
            if (categoryId == string.Empty)
            {
                GetTraderXLogger().LogError(string.Format("[TraderXCategory] Failed to generate category ID for '%1', skipping category", oldCategory.CategoryName));
                continue;
            }

            TraderXCategory category = TraderXCategory.CreateCategory(oldCategory.CategoryName, 0, categoryId);
            foreach(string product: oldCategory.Products)
            {
                TraderXProduct newProduct = oldCategory.GetTraderXProductFromProduct(product);
                if (newProduct)
                {
                    // Save the product individually to ensure it's persisted
                    TraderXProductRepository.Save(newProduct);
                    category.AddProduct(newProduct);
                }
            }
            TraderXCategoryRepository.Save(category);
        }
        GetTraderXLogger().LogInfo("convert done.");
        
        // Move old config file to deleted folder after successful conversion
        MoveOldConfigToDeletedFolder("TraderXPriceConfig.json");
    }
    
    private void MoveOldConfigToDeletedFolder(string fileName)
    {
        string oldFilePath = TRADERX_CONFIG_DIR_SERVER + fileName;
        string deletedDirPath = TRADERX_CONFIG_DIR_SERVER + "deleted\\";
        string newFilePath = deletedDirPath + fileName;
        
        // Create deleted directory if it doesn't exist
        if (!FileExist(deletedDirPath)) {
            MakeDirectory(deletedDirPath);
            GetTraderXLogger().LogInfo("Created deleted directory: " + deletedDirPath);
        }
        
        // Move file to deleted folder
        if (FileExist(oldFilePath)) {
            CopyFile(oldFilePath, newFilePath);
            DeleteFile(oldFilePath);
            GetTraderXLogger().LogInfo("Moved old config file to deleted folder: " + fileName);
        }
    }

	static void ConvertOldConfigToNew()	{
		oldTraderXPriceConfig config = new oldTraderXPriceConfig();
		if (FileExist(TRADERX_CONFIG_DIR_SERVER + "TraderXPriceConfig.json")) {
			GetTraderXLogger().LogInfo("Load old V1 PriceConfig for conversion...");
			TraderXJsonLoader<oldTraderXPriceConfig>.LoadFromFile(TRADERX_CONFIG_DIR_SERVER + "TraderXPriceConfig.json", config);
            GetTraderXLogger().LogInfo("Load done.");
		}

        config.ConvertToNewConfig();
	}
}