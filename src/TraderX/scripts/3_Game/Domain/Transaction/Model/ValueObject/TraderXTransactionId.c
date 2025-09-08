class TraderXTransactionId
{
    static int s_counter = 0;
    private string value;
    
    void TraderXTransactionId(string id = "")
    {
        if (id == "")
            value = "transaction_" + TraderXCoreUtils.GenerateFullTimestamp() + "_" + s_counter.ToString();
        else
            value = id;

        s_counter++;
    }
    
    static TraderXTransactionId Generate()
    {
        return new TraderXTransactionId();
    }
    
    static TraderXTransactionId FromString(string id)
    {
        if (id == "")
            return null;
        return new TraderXTransactionId(id);
    }
    
    string GetValue()
    {
        return value;
    }
    
    bool Equals(TraderXTransactionId other)
    {
        return other && value == other.value;
    }
}