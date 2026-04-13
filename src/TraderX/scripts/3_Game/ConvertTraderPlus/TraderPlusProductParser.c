// ============================================================================
// Parses a TraderPlus V1 product CSV string into a TraderXProduct
// V1 format: "ClassName,Coefficient,MaxStock,TradeQuantity,BuyPrice,SellPrice[,DeStock]"
// ============================================================================

class TraderPlusProductParser
{
    static TraderXProduct Parse(string productCsv)
    {
        TStringArray parts = new TStringArray();
        productCsv.Split(",", parts);

        if (parts.Count() < 1)
            return null;

        string className = parts[0];
        if (className == string.Empty)
            return null;

        float coefficient = 1.0;
        if (parts.Count() > 1)
            coefficient = parts[1].ToFloat();

        int maxStock = -1;
        if (parts.Count() > 2)
            maxStock = parts[2].ToInt();

        int tradeQuantity = 0;
        if (parts.Count() > 3)
            tradeQuantity = PackTradeQuantity(parts[3]);

        int buyPrice = -1;
        if (parts.Count() > 4)
            buyPrice = parts[4].ToInt();

        int sellPrice = -1;
        if (parts.Count() > 5)
            sellPrice = parts[5].ToInt();

        float deStockCoefficient = 0.0;
        if (parts.Count() > 6)
            deStockCoefficient = parts[6].ToFloat();

        string productId = TraderXProductId.GenerateProductId(className);
        if (productId == string.Empty)
        {
            GetTraderXLogger().LogError("[TPV1 Converter] Failed to generate product ID for: " + className);
            return null;
        }

        return TraderXProduct.CreateProduct(
            className,
            coefficient,
            maxStock,
            tradeQuantity,
            buyPrice,
            sellPrice,
            deStockCoefficient,
            0,
            null,
            null,
            productId
        );
    }

    // Packs the V1 single-value trade quantity into TraderX bitfield format
    // V1: -1 = FULL, 0 = EMPTY, <1.0 float = COEFFICIENT, >1 int = STATIC
    private static int PackTradeQuantity(string rawValue)
    {
        int intVal = rawValue.ToInt();
        float floatVal = rawValue.ToFloat();

        int buyMode, sellMode, buyQty, sellQty;

        if (intVal == -1)
        {
            // FULL mode
            buyMode = 0x2;
            sellMode = 0x2;
            buyQty = 0;
            sellQty = 0;
        }
        else if (intVal == 0)
        {
            // EMPTY mode
            buyMode = 0x0;
            sellMode = 0x1;
            buyQty = 0;
            sellQty = 0;
        }
        else if (floatVal > 0.0 && floatVal < 1.0)
        {
            // COEFFICIENT mode
            buyMode = 0x3;
            sellMode = 0x3;
            buyQty = floatVal * 100;
            sellQty = floatVal * 100;
        }
        else
        {
            // STATIC mode
            buyMode = 0x4;
            sellMode = 0x4;
            buyQty = intVal;
            sellQty = intVal;
        }

        return sellMode | (buyMode << 3) | ((sellQty << 6) & 0x1FFF) | (buyQty << 19);
    }
}
