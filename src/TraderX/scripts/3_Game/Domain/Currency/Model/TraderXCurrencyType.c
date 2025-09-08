/**
 * @file TraderXCurrencyType.c
 *
 * @brief This is the implementation file for the TraderXCurrencyType class.
 */

/**
 * @class TraderXCurrencyType
 *
 * @brief The TraderXCurrencyType class represents a type of currency with its name and exchange rate.
 *
 * The TraderXCurrencyType class has functionalities to create and manage a currency type object, as well as adding and removing currencies from it.
 */
class TraderXCurrencyType
{
    string currencyName; /**< Name of the currency type. */
    ref array<ref TraderXCurrency> currencies; /**< Array of currency objects belonging to this currency type. */

    /**
     * Constructor - Initializes the name, exchange rate and an empty array of currencies of this type.
     *
     * @param currencyName Name of the currency type.
     * @param exchangeRate Exchange rate of the currency type.
     */
    void TraderXCurrencyType(string currencyName)
    {
        this.currencyName = currencyName;
        currencies = new array<ref TraderXCurrency>();
    }

    /**
     * Adds a currency object to the array of currencies of this type.
     *
     * @param className Name of the currency.
     * @param value Value of the currency.
     */
    void AddCurrency(string className, int value)
    {
        TraderXCurrency newCurrency = new TraderXCurrency(className, value);
        currencies.Insert(newCurrency);
    }

    /**
     * Sorts the currencies array from highest currency value to lowest.
     */
    void SortCurrenciesByValue()
    {
        for (int i = 0; i < currencies.Count() - 1; i++)
        {
            for (int j = i + 1; j < currencies.Count(); j++)
            {
                if (currencies[i].value < currencies[j].value)
                {
                    TraderXCurrency temp = currencies[i];
                    currencies[i] = currencies[j];
                    currencies[j] = temp;
                }
            }
        }
    }

    TraderXCurrency GetCurrencyForClassName(string className)
    {
        foreach(TraderXCurrency currency : currencies)
        {
            if(CF_String.EqualsIgnoreCase(currency.className, className))
                return currency;
        }

        return null;
    }

}