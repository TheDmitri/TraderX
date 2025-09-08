class TraderXNpcId
{
    private static int s_NextId = 1;
    private int m_Value;

    // Private constructor to enforce use of factory methods
    private void TraderXNpcId(int value)
    {
        if (value <= 0)
        {
            Error("TraderXNpcId must be a positive integer");
            m_Value = 0;
            return;
        }
        m_Value = value;
        
        // Update next available ID if needed
        if (value >= s_NextId)
        {
            s_NextId = value + 1;
        }
    }
    
    // Factory method for creating with existing ID
    static TraderXNpcId Of(int value)
    {
        return new TraderXNpcId(value);
    }
    
    // Factory method for generating a new ID
    static TraderXNpcId Generate()
    {
        return new TraderXNpcId(s_NextId++);
    }
    
    // Get the raw value (use sparingly, prefer methods)
    int Value()
    {
        return m_Value;
    }
    
    // Value object equality
    bool Equals(TraderXNpcId other)
    {
        if (!other) return false;
        return m_Value == other.m_Value;
    }
}