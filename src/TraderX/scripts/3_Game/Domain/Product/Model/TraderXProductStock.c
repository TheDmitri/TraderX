class TraderXProductStock
{
    string productId;
    int stock;
    
    void TraderXProductStock(string productId, int stock)
    {
        this.productId = productId;
        this.stock = stock;
    }

    int GetStock()
    {
        return stock;
    }

    void DecreaseStock()
    {
        if(stock == 0)
            return;
        
        SetStock(stock - 1);
    }

    void IncreaseStock(int multiplier = 1)
    {
        SetStock(stock + 1*multiplier);
    }

    void SetStock(int stock)
    {
        this.stock = stock;
        TraderXProductStockRepository.Save(this);
    }
}

