/**
 * Domain Service: TraderXBitfieldPacker
 * 
 * Responsibility: Pack and unpack bitfield data for TraderX products.
 * 
 * This service encapsulates the complex bitfield logic used to compress
 * multiple product settings into compact integer values for efficient storage.
 * 
 * Bitfield Formats:
 * - tradeQuantity: Packs buy/sell quantity modes and values
 * - stockSettings: Packs destock coefficient and restart behavior
 * 
 * @author TraderX Team
 * @since 2.0 (CSV System)
 */
class TraderXBitfieldPacker
{
    /**
     * Pack trade quantity settings into a single integer.
     * 
     * Bit Layout (32-bit):
     * - Bits 0-2:   Sell mode (3 bits)
     * - Bits 3-5:   Buy mode (3 bits)
     * - Bits 6-15:  Sell value (10 bits, scaled by 1000)
     * - Bits 16-31: Buy value (16 bits, scaled by 1000)
     * 
     * @param buyMode Buy quantity mode (EMPTY, FULL, COEFFICIENT, STATIC)
     * @param buyValue Buy quantity value (0.0-65.535 for COEFFICIENT/STATIC)
     * @param sellMode Sell quantity mode (NO_MATTER, EMPTY, FULL, COEFFICIENT, STATIC)
     * @param sellValue Sell quantity value (0.0-1.023 for COEFFICIENT/STATIC)
     * @return Packed integer containing all trade quantity settings
     */
    static int PackTradeQuantity(string buyMode, float buyValue, string sellMode, float sellValue)
    {
        int packed = 0;
        
        // Pack sell mode (bits 0-2)
        int sellModeInt = 2;
        sellMode.ToUpper();
        
        if (sellMode == "NO_MATTER") sellModeInt = 0;
        else if (sellMode == "EMPTY") sellModeInt = 1;
        else if (sellMode == "FULL") sellModeInt = 2;
        else if (sellMode == "COEFFICIENT") sellModeInt = 3;
        else if (sellMode == "STATIC") sellModeInt = 4;
        
        packed = packed | sellModeInt;
        
        // Pack buy mode (bits 3-5)
        int buyModeInt = 2;
        buyMode.ToUpper();
        
        if (buyMode == "EMPTY") buyModeInt = 1;
        else if (buyMode == "FULL") buyModeInt = 2;
        else if (buyMode == "COEFFICIENT") buyModeInt = 3;
        else if (buyMode == "STATIC") buyModeInt = 4;
        
        packed = packed | (buyModeInt << 3);
        
        // Pack sell value (bits 6-15) - 10 bits, max 1023
        if (sellModeInt == 3 || sellModeInt == 4)
        {
            int sellValueInt = sellValue * 1000.0;
            if (sellValueInt > 1023) sellValueInt = 1023;
            packed = packed | (sellValueInt << 6);
        }
        
        // Pack buy value (bits 16-31) - 16 bits, max 65535
        if (buyModeInt == 3 || buyModeInt == 4)
        {
            int buyValueInt = buyValue * 1000.0;
            if (buyValueInt > 65535) buyValueInt = 65535;
            packed = packed | (buyValueInt << 16);
        }
        
        return packed;
    }
    
    /**
     * Unpack trade quantity settings from a packed integer.
     * 
     * @param packed Packed integer containing trade quantity settings
     * @param buyMode Output: Buy quantity mode
     * @param buyValue Output: Buy quantity value
     * @param sellMode Output: Sell quantity mode
     * @param sellValue Output: Sell quantity value
     */
    static void UnpackTradeQuantity(int packed, out string buyMode, out float buyValue, out string sellMode, out float sellValue)
    {
        // Unpack sell mode (bits 0-2)
        int sellModeInt = packed & 0x7;
        
        switch (sellModeInt)
        {
            case 0: sellMode = "NO_MATTER"; break;
            case 1: sellMode = "EMPTY"; break;
            case 2: sellMode = "FULL"; break;
            case 3: sellMode = "COEFFICIENT"; break;
            case 4: sellMode = "STATIC"; break;
            default: sellMode = "FULL"; break;
        }
        
        // Unpack buy mode (bits 3-5)
        int buyModeInt = (packed >> 3) & 0x7;
        
        switch (buyModeInt)
        {
            case 0: buyMode = "EMPTY"; break;
            case 1: buyMode = "EMPTY"; break;
            case 2: buyMode = "FULL"; break;
            case 3: buyMode = "COEFFICIENT"; break;
            case 4: buyMode = "STATIC"; break;
            default: buyMode = "FULL"; break;
        }
        
        // Unpack sell value (bits 6-15)
        if (sellModeInt == 3 || sellModeInt == 4)
        {
            int sellValueInt = (packed >> 6) & 0x3FF;
            sellValue = sellValueInt / 1000.0;
        }
        else
        {
            sellValue = 0.0;
        }
        
        // Unpack buy value (bits 16-31)
        if (buyModeInt == 3 || buyModeInt == 4)
        {
            int buyValueInt = (packed >> 16) & 0xFFFF;
            buyValue = buyValueInt / 1000.0;
        }
        else
        {
            buyValue = 0.0;
        }
    }
    
    /**
     * Pack stock settings into a single integer.
     * 
     * Bit Layout (32-bit):
     * - Bits 0-15:  Destock coefficient (16 bits, scaled by 10000)
     * - Bits 16-17: Stock behavior at restart (2 bits)
     * - Bits 18-31: Reserved for future use
     * 
     * @param destockCoefficient Destock coefficient (0.0-1.0)
     * @param stockBehaviorAtRestart Behavior code (0-3)
     * @return Packed integer containing stock settings
     */
    static int PackStockSettings(float destockCoefficient, int stockBehaviorAtRestart)
    {
        int packed = 0;
        
        // Pack destock coefficient (bits 0-15) - scaled by 10000
        int coefInt = destockCoefficient * 10000.0;
        if (coefInt > 65535) coefInt = 65535;
        if (coefInt < 0) coefInt = 0;
        
        packed = packed | coefInt;
        
        // Pack stock behavior (bits 16-17)
        int behavior = stockBehaviorAtRestart & 0x3;
        packed = packed | (behavior << 16);
        
        return packed;
    }
    
    /**
     * Unpack stock settings from a packed integer.
     * 
     * @param packed Packed integer containing stock settings
     * @param destockCoefficient Output: Destock coefficient
     * @param stockBehaviorAtRestart Output: Stock behavior code
     */
    static void UnpackStockSettings(int packed, out float destockCoefficient, out int stockBehaviorAtRestart)
    {
        // Unpack destock coefficient (bits 0-15)
        int coefInt = packed & 0xFFFF;
        destockCoefficient = coefInt / 10000.0;
        
        // Unpack stock behavior (bits 16-17)
        stockBehaviorAtRestart = (packed >> 16) & 0x3;
    }
}
