class TraderXTransactionType
{
    private ETraderXTradeMode type;
    
    void TraderXTransactionType(ETraderXTradeMode transactionType)
    {
        type = transactionType;
    }
    
    static TraderXTransactionType CreateBuy()
    {
        return new TraderXTransactionType(ETraderXTradeMode.BUY);
    }
    
    static TraderXTransactionType CreateSell()
    {
        return new TraderXTransactionType(ETraderXTradeMode.SELL);
    }
    
    bool IsBuy()
    {
        return type == ETraderXTradeMode.BUY;
    }
    
    bool IsSell()
    {
        return type == ETraderXTradeMode.SELL;
    }
    
    ETraderXTradeMode GetType()
    {
        return type;
    }
}
