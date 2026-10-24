/**
 * Domain Service: TraderXBitfieldPacker
 *
 * Packs the human-readable CSV columns into the two product bitfields and back.
 * The layouts MUST match what the runtime reads:
 *   - tradeQuantity: 4_World/classes/TraderX/Utilities/TraderXTradeQuantity.c
 *       bits 0-2  sell mode   (0 NO_MATTER, 1 EMPTY, 2 FULL, 3 COEFFICIENT, 4 STATIC)
 *       bits 3-5  buy mode    (0 EMPTY, 2 FULL, 3 COEFFICIENT, 4 STATIC)
 *       bits 6-18 sell value  (COEFFICIENT: percent 0-100, STATIC: units 0-8191)
 *       bits 19+  buy value   (COEFFICIENT: percent 0-100, STATIC: units 0-4095)
 *   - stockSettings: 3_Game/Domain/Product/Model/TraderXProduct.c
 *       bits 0-6  destock percent (0-100), bits 7-8 restart behaviour (0 keep, 1 reset, 2 random)
 *
 * CSV values: COEFFICIENT quantities and destockCoefficient are fractions (0.5 = 50%); a value
 * above 1 is read as a percent (50 = 50%). STATIC quantities are unit counts.
 */
class TraderXBitfieldPacker
{
    static const int MAX_SELL_STATIC = 8191;
    static const int MAX_BUY_STATIC = 4095;

    static int SellModeToInt(string mode)
    {
        mode.ToUpper();
        if (mode == "NO_MATTER") return 0;
        if (mode == "EMPTY") return 1;
        if (mode == "COEFFICIENT") return 3;
        if (mode == "STATIC") return 4;
        return 2;
    }

    static int BuyModeToInt(string mode)
    {
        mode.ToUpper();
        if (mode == "EMPTY") return 0;
        if (mode == "COEFFICIENT") return 3;
        if (mode == "STATIC") return 4;
        return 2;
    }

    static string SellModeToString(int mode)
    {
        switch (mode)
        {
            case 0: return "NO_MATTER";
            case 1: return "EMPTY";
            case 3: return "COEFFICIENT";
            case 4: return "STATIC";
        }
        return "FULL";
    }

    static string BuyModeToString(int mode)
    {
        switch (mode)
        {
            case 0: return "EMPTY";
            case 3: return "COEFFICIENT";
            case 4: return "STATIC";
        }
        return "FULL";
    }

    static int ClampInt(int value, int min, int max)
    {
        if (value < min) return min;
        if (value > max) return max;
        return value;
    }

    // Fraction (0.5) or percent (50) -> percent 0-100
    static int ToPercent(float value)
    {
        float pct = value;
        if (value <= 1.0)
            pct = value * 100.0;
        int rounded = Math.Round(pct);
        return ClampInt(rounded, 0, 100);
    }

    static int PackTradeQuantity(string buyMode, float buyValue, string sellMode, float sellValue)
    {
        int sellModeInt = SellModeToInt(sellMode);
        int buyModeInt = BuyModeToInt(buyMode);

        int sellQty = 0;
        if (sellModeInt == 3)
            sellQty = ToPercent(sellValue);
        else if (sellModeInt == 4)
        {
            sellQty = Math.Round(sellValue);
            sellQty = ClampInt(sellQty, 0, MAX_SELL_STATIC);
        }

        int buyQty = 0;
        if (buyModeInt == 3)
            buyQty = ToPercent(buyValue);
        else if (buyModeInt == 4)
        {
            buyQty = Math.Round(buyValue);
            buyQty = ClampInt(buyQty, 0, MAX_BUY_STATIC);
        }

        return sellModeInt | (buyModeInt << 3) | ((sellQty & 0x1FFF) << 6) | (buyQty << 19);
    }

    static void UnpackTradeQuantity(int packed, out string buyMode, out float buyValue, out string sellMode, out float sellValue)
    {
        int sellModeInt = packed & 0x7;
        int buyModeInt = (packed >> 3) & 0x7;
        int sellQty = (packed >> 6) & 0x1FFF;
        int buyQty = packed >> 19;

        sellMode = SellModeToString(sellModeInt);
        buyMode = BuyModeToString(buyModeInt);

        sellValue = 0.0;
        if (sellModeInt == 3)
            sellValue = ClampInt(sellQty, 0, 100) / 100.0;
        else if (sellModeInt == 4)
            sellValue = sellQty;

        buyValue = 0.0;
        if (buyModeInt == 3)
            buyValue = ClampInt(buyQty, 0, 100) / 100.0;
        else if (buyModeInt == 4)
            buyValue = buyQty;
    }

    static int PackStockSettings(float destockCoefficient, int stockBehaviorAtRestart)
    {
        int destockPct = ToPercent(destockCoefficient);
        return (destockPct & 0x7F) | ((stockBehaviorAtRestart & 0x3) << 7);
    }

    static void UnpackStockSettings(int packed, out float destockCoefficient, out int stockBehaviorAtRestart)
    {
        int destockPct = packed & 0x7F;
        destockCoefficient = ClampInt(destockPct, 0, 100) / 100.0;
        stockBehaviorAtRestart = (packed >> 7) & 0x3;
    }
}
