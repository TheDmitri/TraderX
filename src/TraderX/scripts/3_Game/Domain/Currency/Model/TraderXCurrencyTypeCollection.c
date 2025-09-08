/**
 * @file TraderXCurrencyTypeCollection.c
 *
 * @brief This is the implementation file for the TraderXCurrencyTypeCollection class.
 */

/**
 * @class TraderXCurrencyTypeCollection
 *
 * @brief The TraderXCurrencyTypeCollection class handles exchange rates between different currencies.
 *
 * The TraderXCurrencyExchange class has functionalities to add currency type,
 * convert currency and get cross rate of currencies.
 */
class TraderXCurrencyTypeCollection
{
    string version = TRADERX_CURRENT_VERSION;
    ref array<ref TraderXCurrencyType> currencyTypes; /**< An array to store different currency types. */

    /**
     * Constructor - Initializes the array of currencies.
     */
    void TraderXCurrencyTypeCollection()
    {
        currencyTypes = new array<ref TraderXCurrencyType>();
    }

    /**
     * Adds a new currency type to the list of known currencies.
     *
     * @param currencyName Name of the new currency type.
     */
    void AddCurrencyType(string currencyName)
    {
        TraderXCurrencyType newCurrencyType = new TraderXCurrencyType(currencyName);
        currencyTypes.Insert(newCurrencyType);
    }

    /**
     * Gets the currency type object for a given currency name.
     *
     * @param currencyName Currency name of the currency type.
     *
     * @returns Returns the currency type object for the given currency name.
     */
    TraderXCurrencyType GetCurrencyType(string currencyName)
    {
        foreach (TraderXCurrencyType type : currencyTypes)
        {
            if (type.currencyName == currencyName)
                return type;
        }

        return null; // type not found
    }

    void DefaultCurrencyTypeCollection()
	{
        TraderXCurrencyType currencyType1 = new TraderXCurrencyType("EUR");
        currencyType1.AddCurrency("TraderX_Money_Euro100", 100);
        currencyType1.AddCurrency("TraderX_Money_Euro50", 50);
        currencyType1.AddCurrency("TraderX_Money_Euro10", 10);
        currencyType1.AddCurrency("TraderX_Money_Euro5", 5);
        currencyType1.AddCurrency("TraderX_Money_Euro1", 1);
        TraderXCurrencyType currencyType2 = new TraderXCurrencyType("USD");
        currencyType1.AddCurrency("TraderX_Money_Dollar100", 100);
        currencyType1.AddCurrency("TraderX_Money_Dollar50", 50);
        currencyType1.AddCurrency("TraderX_Money_Dollar10", 10);
        currencyType1.AddCurrency("TraderX_Money_Dollar5", 5);
        currencyType1.AddCurrency("TraderX_Money_Dollar1", 1);
        currencyTypes.Insert(currencyType1);
        currencyTypes.Insert(currencyType2);
	}

    string ToCollectionString()
    {
        string result = "";
        result += "{";
        result += "\n  version: " + version;
        result += "\n  currencyTypes: [";
        foreach (TraderXCurrencyType type : currencyTypes)
        {
            result += "\n    " + type.currencyName;
            result += "\n    currencies: [";
            foreach (TraderXCurrency currency : type.currencies)
            {
                result += "\n      " + currency.className + " (" + currency.value + ")";
            }
            result += "\n    ]";
        }
        result += "\n  ]";
        result += "\n}";
        return result;
    }
}
