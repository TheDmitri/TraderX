class TraderXProductMapper
{
    static TraderXProduct MapToTraderXProduct(TraderXJsonProduct jsonProduct)
    {
        TraderXProduct tProduct = new TraderXProduct();
        tProduct.className = jsonProduct.className;
        tProduct.coefficient = jsonProduct.coefficient;
        tProduct.maxStock = jsonProduct.maxStock;
        tProduct.tradeQuantity = jsonProduct.tradeQuantity;
        tProduct.buyPrice = jsonProduct.buyPrice;
        tProduct.sellPrice = jsonProduct.sellPrice;
        tProduct.stockSettings = jsonProduct.stockSettings;
        tProduct.attachments = jsonProduct.attachments;
        tProduct.variants = jsonProduct.variants;
        return tProduct;
    }

    static TraderXJsonProduct MapToTraderXJsonProduct(TraderXProduct tProduct)
    {
        TraderXJsonProduct jsonProduct = new TraderXJsonProduct();
        jsonProduct.className = tProduct.className;
        jsonProduct.coefficient = tProduct.coefficient;
        jsonProduct.maxStock = tProduct.maxStock;
        jsonProduct.tradeQuantity = tProduct.tradeQuantity;
        jsonProduct.buyPrice = tProduct.buyPrice;
        jsonProduct.sellPrice = tProduct.sellPrice;
        jsonProduct.stockSettings = tProduct.stockSettings;
        jsonProduct.attachments = tProduct.attachments;
        jsonProduct.variants = tProduct.variants;
        return jsonProduct;
    }
}