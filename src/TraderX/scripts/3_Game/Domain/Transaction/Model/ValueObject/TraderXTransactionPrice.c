class TraderXTransactionPrice
{
    private int amount;
    
    void TraderXTransactionPrice(int price)
    {
        if (price < 0)
            amount = 0;
        else
            amount = price;
    }
    
    static TraderXTransactionPrice CreateZero()
    {
        return new TraderXTransactionPrice(0);
    }
    
    static TraderXTransactionPrice CreateFromAmount(int price)
    {
        return new TraderXTransactionPrice(price);
    }
    
    int GetAmount()
    {
        return amount;
    }
    
    bool IsZero()
    {
        return amount == 0;
    }
    
    bool IsPositive()
    {
        return amount > 0;
    }
    
    bool IsValidPrice()
    {
        return amount >= 0;
    }
    
    TraderXTransactionPrice Multiply(int factor)
    {
        return new TraderXTransactionPrice(amount * factor);
    }
    
    TraderXTransactionPrice Add(TraderXTransactionPrice other)
    {
        if (!other)
            return new TraderXTransactionPrice(amount);
        return new TraderXTransactionPrice(amount + other.amount);
    }
    
    string ToStringFormatted()
    {
        return amount.ToString() + " EUR";
    }
}