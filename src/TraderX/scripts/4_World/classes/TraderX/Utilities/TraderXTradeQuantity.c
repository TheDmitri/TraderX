class TraderXTradeQuantity
{
  static const int SELL_NO_MATTER = 0x0;
  static const int SELL_EMPTY = 0x1;
  static const int SELL_FULL = 0x2;
  static const int SELL_COEFFICIENT = 0x3;
  static const int SELL_STATIC = 0x4;
  static const int BUY_EMPTY = 0x0;
  static const int BUY_FULL = 0x2;
  static const int BUY_COEFFICIENT = 0x3;
  static const int BUY_STATIC = 0x4;

  static int GetItemBuyQuantity(string className, int tradeQuantity)
  {
    int tradeQuantityMode = GetBuyQuantityMode(tradeQuantity);

    int quantity = 0;
    switch(tradeQuantityMode)
    {
        case BUY_EMPTY: quantity = 0;
        break;

        case BUY_FULL: quantity = TraderXQuantityManager.GetMaxItemQuantityServer(className);
        break;

        case BUY_COEFFICIENT: quantity = TraderXQuantityManager.GetMaxItemQuantityServer(className) * GetBuyQuantityCoefficient(tradeQuantity);
        break;

        case BUY_STATIC: quantity = tradeQuantity >> 19;
    }

    return quantity;
  }

  static int GetItemBuyQuantityClient(EntityAI item, int tradeQuantity)
  {
    int tradeQuantityMode = GetBuyQuantityMode(tradeQuantity);

    int quantity = 0;
    switch(tradeQuantityMode)
    {
        case BUY_EMPTY: quantity = 0;
        break;

        case BUY_FULL: quantity = TraderXQuantityManager.GetMaxItemQuantityClient(item);
        break;

        case BUY_COEFFICIENT: quantity = TraderXQuantityManager.GetMaxItemQuantityClient(item) * GetBuyQuantityCoefficient(tradeQuantity);
        break;

        case BUY_STATIC: quantity = tradeQuantity >> 19;
    }

    return quantity;
  }

  static int GetItemSellQuantity(string className, int tradeQuantity, int currentAmount = 0)
  {
    int tradeQuantityMode = GetSellQuantityMode(tradeQuantity);

    int quantity = 0;
    switch(tradeQuantityMode)
    {
        case SELL_NO_MATTER: {
          quantity = currentAmount;
        }
        break;

        case SELL_EMPTY: quantity = 0;
        break;

        case SELL_FULL: quantity = TraderXQuantityManager.GetMaxItemQuantityServer(className);
        break;

        case SELL_COEFFICIENT: {
          quantity = TraderXQuantityManager.GetMaxItemQuantityServer(className) * GetSellQuantityCoefficient(tradeQuantity);
        }
        break;

        case SELL_STATIC: {
          quantity = (tradeQuantity >> 6) & 0x1FFF;
        }
        break;
    }

    return quantity;
  }

  static int CreateTradeQuantity(int buyMode, int sellMode, int buyQuantity, int sellQuantity)
  {
    return sellMode | (buyMode << 3) | ((sellQuantity << 6) & 0x1FFF) | (buyQuantity << 19);
  }

  static int GetBuyQuantityMode(int tradeQuantity)
  {
    return (tradeQuantity >> 3) & 0x7;
  }

  static int GetSellQuantityMode(int tradeQuantity)
  {
    return tradeQuantity & 0x7;
  }

  static float GetSellQuantityCoefficient(int tradeQuantity)
  {
    int coefficientInt = (tradeQuantity >> 6) & 0x1FFF;

    if(coefficientInt > 100){
      coefficientInt = 100;
    }

    return coefficientInt / 100.0;
  }

  static float GetBuyQuantityCoefficient(int tradeQuantity)
  {
    int coefficientInt = tradeQuantity >> 19;

    if(coefficientInt > 100){
      coefficientInt = 100;
    }

    return coefficientInt / 100.0;
  }
}