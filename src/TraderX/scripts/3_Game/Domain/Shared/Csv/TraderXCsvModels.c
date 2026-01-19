// TraderX CSV Data Models - Structures for CSV data before compilation

// CSV Product Model (human-readable, unpacked fields)
class TraderXCsvProduct
{
    string productKey;                  // Unique identifier (e.g., "akm", "mag_akm_30")
    string className;                   // DayZ class name (e.g., "AKM")
    int buyPrice;                       // Price player pays to buy
    int sellPrice;                      // Price player receives when selling
    int maxStock;                       // Maximum stock level (-1 = unlimited)
    
    // Trade quantity fields (unpacked from tradeQuantity bitfield)
    string buyQtyMode;                  // "EMPTY", "FULL", "COEFFICIENT", "STATIC"
    float buyQtyValue;                  // Value for COEFFICIENT/STATIC modes
    string sellQtyMode;                 // "NO_MATTER", "EMPTY", "FULL", "COEFFICIENT", "STATIC"
    float sellQtyValue;                 // Value for COEFFICIENT/STATIC modes
    
    // Stock settings fields (unpacked from stockSettings bitfield)
    float destockCoefficient;           // Rate of stock decrease (0.0-1.0)
    int stockBehaviorAtRestart;         // 0=keep, 1=reset, 2=partial
    
    // References
    ref array<string> attachments;          // Semicolon-separated productKeys
    ref array<string> variants;             // Semicolon-separated productKeys
    
    string notes;                       // Admin notes (not loaded by game)
    
    void TraderXCsvProduct()
    {
        productKey = "";
        className = "";
        buyPrice = 0;
        sellPrice = 0;
        maxStock = 0;
        buyQtyMode = "FULL";
        buyQtyValue = 0.0;
        sellQtyMode = "FULL";
        sellQtyValue = 0.0;
        destockCoefficient = 0.0;
        stockBehaviorAtRestart = 0;
        notes = "";
    }
}

// CSV Category Model
class TraderXCsvCategory
{
    string categoryKey;                 // Unique identifier (e.g., "weapons_assault")
    string categoryName;                // Display name (e.g., "Assault Rifles")
    string icon;                        // Icon identifier (e.g., "icon_weapons")
    bool isVisible;                     // Show in trader menu (1=visible, 0=hidden)
    ref array<string> licensesRequired;     // Semicolon-separated license names
    ref array<string> productKeys;          // Semicolon-separated productKeys
    string notes;                       // Admin notes
    
    void TraderXCsvCategory()
    {
        categoryKey = "";
        categoryName = "";
        icon = "";
        isVisible = true;
        notes = "";
    }
}

// CSV Preset Model
class TraderXCsvPreset
{
    string presetKey;                   // Unique identifier (e.g., "preset_akm_tactical")
    string productKey;                  // Reference to product
    string presetName;                  // Display name (e.g., "Tactical Loadout")
    ref array<string> attachments;          // Semicolon-separated attachment productKeys
    string notes;                       // Admin notes
    
    void TraderXCsvPreset()
    {
        presetKey = "";
        productKey = "";
        presetName = "";
        notes = "";
    }
}

// Validation Error/Warning Model
class TraderXValidationError
{
    string errorType;                   // "ERROR" or "WARNING"
    string source;                      // "products.csv", "categories.csv", etc.
    int row;                            // Row number in CSV (1-indexed, 0 = header)
    string column;                      // Column name
    string message;                     // Error description
    string value;                       // Problematic value
    
    void TraderXValidationError(string type, string src, int rowNum, string col, string msg, string val)
    {
        errorType = type;
        source = src;
        row = rowNum;
        column = col;
        message = msg;
        value = val;
    }
}

// CSV Pack Metadata (for multi-file support)
class TraderXCsvPackInfo
{
    string fileName;                    // e.g., "50_community_weapons.csv"
    int priority;                       // Derived from filename prefix (50)
    string packName;                    // e.g., "community_weapons"
    
    void TraderXCsvPackInfo(string file)
    {
        fileName = file;
        
        // Extract priority from filename (e.g., "50_" -> 50)
        int underscorePos = file.IndexOf("_");
        if (underscorePos > 0)
        {
            string priorityStr = file.Substring(0, underscorePos);
            priority = priorityStr.ToInt();
            packName = file.Substring(underscorePos + 1, file.Length() - underscorePos - 5); // Remove "_" and ".csv"
        }
        else
        {
            priority = 0;
            packName = file;
        }
    }
}
