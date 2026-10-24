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

    // TraderPlus prices with coefficient 1 when it is 0 or negative (CalculatePriceForThatItem).
    float GetCoefficientFromProduct(string product)
    {
        TStringArray arr = GetStringArrayFromString(product);
        if(arr.Count() < 2)
            return 1.0;

        float coefficient = arr[1].ToFloat();
        if (coefficient <= 0)
            return 1.0;
        return coefficient;
    }

    int GetMaxStockFromProduct(string product)
    {
       TStringArray arr = GetStringArrayFromString(product);
        if(arr.Count() < 3)
            return -1;

        return arr[2].ToFloat();
    }

    // Max quantity exactly as TraderPlus reads it (TraderPlusHelper.GetMaxItemQuantityClient).
    static int GetTraderPlusMaxQuantity(string className)
    {
        string path = CFG_VEHICLESPATH + " " + className + " varQuantityMax";
        if (GetGame().ConfigIsExisting(path))
            return GetGame().ConfigGetInt(path);

        path = CFG_WEAPONSPATH + " " + className + " varQuantityMax";
        if (GetGame().ConfigIsExisting(path))
            return GetGame().ConfigGetInt(path);

        path = CFG_MAGAZINESPATH + " " + className + " varQuantityMax";
        if (className.Contains("Ammo_") || className.Contains("Mag_"))
            path = CFG_MAGAZINESPATH + " " + className + " count";
        if (GetGame().ConfigIsExisting(path))
            return GetGame().ConfigGetInt(path);

        return 0;
    }

    // TraderPlus resolves TradeQty against the item's max quantity (TraderClient.CalculateTradeQty) and
    // uses the result for buying and selling: above the max = full, magazines are traded empty (the
    // ammo stays with the player), 0<x<1 = that share of the max, -1 = full, 0 = empty / any amount,
    // otherwise that many units. The TraderX modes reproduce the resolved quantity.
    int GetTradeQuantityFromProduct(string product)
    {
        TStringArray arr = GetStringArrayFromString(product);
        if(arr.Count() < 4)
            return CreateTradeQuantity(0x2, 0x2, 0, 0);

        string className = GetClassNameFromProduct(product);
        string raw = arr[3];
        raw.TrimInPlace();
        float quantity = raw.ToFloat();
        int maxQuantity = GetTraderPlusMaxQuantity(className);

        if (quantity == 0.0)
            return CreateTradeQuantity(0x0, 0x1, 0, 0);

        if (quantity > maxQuantity)
            return CreateTradeQuantity(0x2, 0x2, 0, 0);

        if (className.Contains("Mag_"))
            return CreateTradeQuantity(0x0, 0x1, 0, 0);

        if (quantity == -1.0)
            return CreateTradeQuantity(0x2, 0x2, 0, 0);

        int amount = quantity;
        if (quantity > 0.0 && quantity < 1.0)
            amount = quantity * maxQuantity;

        if (amount <= 0)
            return CreateTradeQuantity(0x0, 0x1, 0, 0);
        if (amount >= maxQuantity)
            return CreateTradeQuantity(0x2, 0x2, 0, 0);

        // A fixed amount, like TraderPlus (a share would apply to TraderX's max, which can differ from the config one)
        if (amount <= 4095)
            return CreateTradeQuantity(0x4, 0x4, amount, amount);

        int percent = Math.Round(amount * 100.0 / maxQuantity);
        int byPercent = maxQuantity * (percent / 100.0);
        TraderXMigrationReport.Approx("TraderPlus", string.Format("%1: trade quantity %2 of %3 above the TraderX fixed-amount maximum, traded as %4 percent (%5)", className, amount, maxQuantity, percent, byPercent));
        return CreateTradeQuantity(0x3, 0x3, percent, percent);
    }

    int CreateTradeQuantity(int buyMode, int sellMode, int buyQuantity, int sellQuantity)
    {
        return sellMode | (buyMode << 3) | ((sellQuantity & 0x1FFF) << 6) | (buyQuantity << 19);
    }

    int GetBuyPriceFromProduct(string product)
    {
        TStringArray arr = GetStringArrayFromString(product);
        if(arr.Count() < 5)
            return -1;

        return arr[4].ToInt();
    }

    // TraderPlus SellPrice: -1 = not sellable, below 1 = ratio of the buy price, otherwise a price.
    int GetSellPriceFromProduct(string product)
    {
        TStringArray arr = GetStringArrayFromString(product);
        if(arr.Count() < 6)
            return -1;

        float sell = arr[5].ToFloat();
        if (sell == -1.0)
            return -1;

        if (sell < 1.0)
            sell = arr[4].ToFloat() * sell;

        // TraderPlus stores the price in an int: the decimals are dropped, not rounded
        int price = sell;
        if (price < 0)
            return -1;
        return price;
    }

    // TraderPlus only destocks at restart when EnableAutoDestockAtRestart is on; a line without a
    // 7th column is then wiped (coefficient 1).
    float GetDeStockCoefficientFromProduct(string product, bool autoDestock)
    {
        if (!autoDestock)
            return 0.0;

        TStringArray arr = GetStringArrayFromString(product);
        if(arr.Count() < 7)
            return 1.0;

        return arr[6].ToFloat();
    }

    TraderXProduct GetTraderXProductFromProduct(string product, int stockBehaviorAtRestart = 0, bool autoDestock = false)
    {
        string className = GetClassNameFromProduct(product);
        float coefficient= GetCoefficientFromProduct(product);
        int maxStock = GetMaxStockFromProduct(product);
        int tradeQuantity = GetTradeQuantityFromProduct(product);
        int buyPrice = GetBuyPriceFromProduct(product);
        int sellPrice = GetSellPriceFromProduct(product);
        float deStockCoefficient = GetDeStockCoefficientFromProduct(product, autoDestock);

        // Generate a unique product ID based on the class name
        string productId = TraderXProductId.GenerateProductId(className);
        if (productId == string.Empty)
        {
            GetTraderXLogger().LogError(string.Format("[TraderXProduct] Failed to generate product ID for class '%1'", className));
            return null;
        }

        return TraderXProduct.CreateProduct(className, coefficient, maxStock, tradeQuantity, buyPrice, sellPrice, deStockCoefficient, stockBehaviorAtRestart, null, null, productId);
    }
};

class oldTraderXPriceConfig
{
    string Version = "";
	bool EnableAutoCalculation;
	bool EnableAutoDestockAtRestart;
	int EnableDefaultTraderStock;
	ref array<ref oldTraderXCategory>TraderCategories;

	// Path the config was actually loaded from (TraderX dir or TraderPlus dir) — archived after convert.
	string sourcePath = "";

	void oldTraderXPriceConfig()
	{
		TraderCategories = new array<ref oldTraderXCategory>;
	}

    void ConvertToNewConfig()
    {
        GetTraderXLogger().LogInfo("ConvertToNewConfig");
        if (EnableAutoCalculation)
            TraderXMigrationReport.Skipped("TraderPlus", "EnableAutoCalculation (was not functional in TraderPlus either)");
        if (EnableAutoDestockAtRestart)
            TraderXMigrationReport.Approx("TraderPlus", "EnableAutoDestockAtRestart: TraderPlus removed stock*coefficient only when stock reached max; TraderX keeps stock*(1-coefficient) at every restart");

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
                TraderXProduct newProduct = oldCategory.GetTraderXProductFromProduct(product, EnableDefaultTraderStock, EnableAutoDestockAtRestart);
                if (newProduct)
                {
                    // Save the product individually to ensure it's persisted
                    TraderXProductRepository.Save(newProduct);
                    category.AddProduct(newProduct);
                    ConvertTraderPlusPlayerData.RegisterProduct(oldCategory.CategoryName, newProduct);
                }
            }
            TraderXCategoryRepository.Save(category);
        }
        GetTraderXLogger().LogInfo("convert done.");

        // Archive the consumed source so the migration won't run again.
        TraderXLegacyConfigPath.Archive(sourcePath, "TraderPlusPriceConfig.json");
    }

	static void ConvertOldConfigToNew()	{
		string path = TraderXLegacyConfigPath.Resolve("TraderXPriceConfig.json", "TraderPlusPriceConfig.json");
		if (path == string.Empty)
			return;

		oldTraderXPriceConfig config = new oldTraderXPriceConfig();
		GetTraderXLogger().LogInfo("[TraderX Migration] Loading legacy price config: " + path);
		TraderXJsonLoader<oldTraderXPriceConfig>.LoadFromFile(path, config);
		config.sourcePath = path;
		config.ConvertToNewConfig();
	}
}