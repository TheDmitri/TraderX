// TODO: Consider standardizing enum naming to ETraderXX format
// For example, rename ETradeMode to ETraderXTradeMode for consistency

enum ETraderXTradeMode
{
  SELL,
  BUY
}

// TODO: Consider renaming to ETraderXTradeMode for consistency with other enums
enum ETradeMode
{
  BUY,
  SELL,
  REPLACED
}

enum ESellMode
{
  ENTIRE_CONTAINER = 0,
  SELECTED_ITEM,
  MANUAL
}

enum ETraderXResponse
{
  TRANSACTIONS,
  ALL_STOCK_RECEIVED,
  STOCK_UPDATE
};
