class TraderXNetworkIdentifier
{
    private int lowId;
    private int highId;
    
    void TraderXNetworkIdentifier(int low = 0, int high = 0)
    {
        lowId = low;
        highId = high;
    }
    
    static TraderXNetworkIdentifier CreateEmpty()
    {
        return new TraderXNetworkIdentifier(0, 0);
    }
    
    static TraderXNetworkIdentifier CreateFromIds(int low, int high)
    {
        return new TraderXNetworkIdentifier(low, high);
    }
    
    bool IsEmpty()
    {
        return lowId == 0 && highId == 0;
    }
    
    int GetLowId()
    {
        return lowId;
    }
    
    int GetHighId()
    {
        return highId;
    }
    
    bool Equals(TraderXNetworkIdentifier other)
    {
        return other && lowId == other.lowId && highId == other.highId;
    }
}