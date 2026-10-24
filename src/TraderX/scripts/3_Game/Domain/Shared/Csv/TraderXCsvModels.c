// TraderX CSV Data Models - Structures for CSV data before compilation

// CSV Product Model (human-readable, unpacked fields)
class TraderXCsvProduct
{
    string productKey;                  // Unique identifier = productId (e.g., "prod_akm_001")
    string className;                   // DayZ class name (e.g., "AKM")
    int buyPrice;                       // Price player pays to buy (-1 = not for sale)
    int sellPrice;                      // Price player receives when selling (-1 = cannot be sold)
    int maxStock;                       // Maximum stock level (-1 = unlimited)
    float coefficient;                  // Price decay per unit in stock: price = base * coefficient^(stock-1); 1 = flat

    // Trade quantity fields (unpacked from tradeQuantity bitfield)
    string buyQtyMode;                  // "EMPTY", "FULL", "COEFFICIENT", "STATIC"
    float buyQtyValue;                  // COEFFICIENT: fraction of max (0.5 = 50%), STATIC: units
    string sellQtyMode;                 // "NO_MATTER", "EMPTY", "FULL", "COEFFICIENT", "STATIC"
    float sellQtyValue;                 // COEFFICIENT: fraction of max (0.5 = 50%), STATIC: units

    // Stock settings fields (unpacked from stockSettings bitfield)
    float destockCoefficient;           // Share of stock removed at each restart (0.1 = 10%)
    int stockBehaviorAtRestart;         // 0=keep, 1=reset to maxStock, 2=random 0..maxStock

    // References
    ref array<string> attachments;          // Semicolon-separated productKeys
    ref array<string> variants;             // Semicolon-separated productKeys

    string notes;                       // Admin notes (not loaded by game)

    // Where the row came from, for the validation report
    string sourceFile;
    int sourceRow;

    void TraderXCsvProduct()
    {
        productKey = "";
        className = "";
        buyPrice = -1;
        sellPrice = -1;
        maxStock = -1;
        coefficient = 1.0;
        buyQtyMode = "FULL";
        buyQtyValue = 0.0;
        sellQtyMode = "FULL";
        sellQtyValue = 0.0;
        destockCoefficient = 0.0;
        stockBehaviorAtRestart = 0;
        notes = "";
        sourceFile = "";
        sourceRow = 0;
    }
}

// CSV Category Model
class TraderXCsvCategory
{
    string categoryKey;                 // Unique identifier = categoryId (e.g., "cat_assault_rifles_001")
    string categoryName;                // Display name (e.g., "Assault Rifles")
    string icon;                        // Icon identifier (e.g., "icon_weapons")
    bool isVisible;                     // Show in trader menu (1=visible, 0=hidden)
    ref array<string> licensesRequired;     // Semicolon-separated license names
    ref array<string> productKeys;          // Semicolon-separated productKeys
    string notes;                       // Admin notes
    string sourceFile;
    int sourceRow;

    void TraderXCsvCategory()
    {
        categoryKey = "";
        categoryName = "";
        icon = "";
        isVisible = true;
        notes = "";
        sourceFile = "";
        sourceRow = 0;
    }
}

// CSV Preset Model
class TraderXCsvPreset
{
    string presetKey;                   // Unique identifier = presetId (e.g., "preset_akm_tactical")
    string productKey;                  // Reference to product
    string presetName;                  // Display name (e.g., "Tactical Loadout")
    bool isDefault;                     // Default preset of the product (first preset of the product if none)
    ref array<string> attachments;          // Semicolon-separated attachment productKeys
    string notes;                       // Admin notes
    string sourceFile;
    int sourceRow;

    void TraderXCsvPreset()
    {
        presetKey = "";
        productKey = "";
        presetName = "";
        isDefault = false;
        notes = "";
        sourceFile = "";
        sourceRow = 0;
    }
}

// Validation Error/Warning Model
class TraderXValidationError
{
    string errorType;                   // "ERROR" or "WARNING"
    string source;                      // "products.csv", "categories.csv", etc.
    int row;                            // Line number in the source file (1 = header)
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
