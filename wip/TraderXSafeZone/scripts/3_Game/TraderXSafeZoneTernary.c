class TraderXSafeZoneTernary<Class T>
{
    private void TraderXSafeZoneTernary();
    private void ~TraderXSafeZoneTernary();
    
    // Generic ternary for any class type
    static T If(bool condition, T trueValue, T falseValue)
    {
        if (condition)
        {
            return trueValue;
        }
        return falseValue;
    }
}