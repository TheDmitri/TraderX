class TraderXCategoryMapper
{
    static TraderXCategory MapToTraderXCategory(TraderXJsonCategory jsonCategory)
    {
        TraderXCategory tCategory = new TraderXCategory();
        tCategory.isVisible = jsonCategory.isVisible;
        tCategory.icon = jsonCategory.icon;
        tCategory.categoryName = jsonCategory.categoryName;
        tCategory.licensesRequired = jsonCategory.licensesRequired;
        tCategory.productIds = jsonCategory.productIds;
        return tCategory;
    }

    static TraderXJsonCategory MapToTraderXJsonCategory(TraderXCategory tCategory)
    {
        TraderXJsonCategory jsonCategory = new TraderXJsonCategory();
        jsonCategory.isVisible = tCategory.isVisible;
        jsonCategory.icon = tCategory.icon;
        jsonCategory.categoryName = tCategory.categoryName;
        jsonCategory.licensesRequired = tCategory.licensesRequired;
        jsonCategory.productIds = tCategory.productIds;
        return jsonCategory;
    }
}
