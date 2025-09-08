class TraderXJsonCategory
{
    bool isVisible = true;
    string icon;
    string categoryName;
    ref array<string> licensesRequired;
    ref array<string> productIds;
}

class TraderXCategory extends TraderXJsonCategory
{
    string categoryId = string.Empty;

    [NonSerialized()]
    int categoryType = -1;

    void TraderXCategory(string categoryName = string.Empty, int categoryType = ETraderXCategoryType.NONE, string categoryId = string.Empty)
    {
        this.productIds = new array<string>();
        this.licensesRequired = new array<string>();
    }

    static TraderXCategory CreateCategory(string _categoryName = string.Empty, int _categoryType = ETraderXCategoryType.NONE, string _categoryId = string.Empty)
    {
        TraderXCategory category = new TraderXCategory();
        category.categoryName = _categoryName;
        category.categoryType = _categoryType;
        TraderXCategoryId.AssignIdIfNot(category, _categoryId);

        return category;
    }

    void AddProduct(TraderXProduct product)
    {
        if (!product || !productIds)
            return;

        if (productIds.Find(product.productId) == -1)
        {
            productIds.Insert(product.productId);
            TraderXProductRepository.Save(product);
        }
    }

    void RemoveProduct(TraderXProduct product)
    {
        if (!product || !productIds)
            return;

        int idx = productIds.Find(product.productId);
        if (idx != -1)
        {
            productIds.Remove(idx);
        }
    }

    array<ref TraderXProduct> GetProducts()
    {
        array<ref TraderXProduct> products = new array<ref TraderXProduct>();
        foreach (string productId : productIds)
        {
            TraderXProduct product = TraderXProductRepository.GetItemById(productId);
            if (product)
                products.Insert(product);
        }
        return products;
    }

    TraderXProduct FindProductByClassName(string className)
    {
        array<ref TraderXProduct> products = GetProducts();
        foreach(TraderXProduct product: products)
        {
            if(CF_String.EqualsIgnoreCase(product.className, className)){
                GetTraderXLogger().LogDebug("FindProductByClassName : " + product.className);
                return product;
            }
        }

        return null;
    }

    TraderXProduct GetItem(string id)
    {
        array<ref TraderXProduct> products = GetProducts();
        foreach(TraderXProduct product: products)
        {
            if(product.productId == id)
                return product;
        }
        return null;
    }

    // Security method: Check if category contains a specific product
    bool ContainsProduct(string productId)
    {
        if (!productIds)
            return false;
            
        return productIds.Find(productId) != -1;
    }
};