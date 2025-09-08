/**
 * @class TraderXCurrency
 *
 * @brief The TraderXCurrency class represents a currency with its name and value.
 *
 * The TraderXCurrency class has functionalities to create and manage a currency object.
 */
class TraderXCurrency
{
	string className; /**< Name of the currency. */
	int value; /**< Value of the currency. */

	/**
	 * Constructor - Initializes the name and value of the currency.
	 *
	 * @param className Name of the currency.
	 * @param value Value of the currency.
	 */
	void TraderXCurrency(string className, int value)
	{
		this.className = className;
		this.value = value;
	}

	int GetCurrencyValue()
	{
		return value;
	}

	string GetCurrencyClassName()
	{
		return className;
	}
}

