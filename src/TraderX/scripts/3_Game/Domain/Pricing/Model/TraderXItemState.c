// Domain value object for item state multipliers
class TraderXItemState
{
    const int PRISTINE = 0;
    const int WORN = 1;
    const int DAMAGED = 2;
    const int BADLY_DAMAGED = 3;
    const int RUINED = 4;
    
    static float GetStateMultiplier(int itemState)
    {
        switch (itemState)
        {
            case TraderXItemState.PRISTINE:
                return 1.0;
            case TraderXItemState.WORN:
                return 0.8;
            case TraderXItemState.DAMAGED:
                return 0.6;
            case TraderXItemState.BADLY_DAMAGED:
                return 0.4;
            case TraderXItemState.RUINED:
                return 0.0; // Ruined items cannot be traded
            default:
                return 1.0;
        }
        
        return 1.0; // Fallback return statement
    }
    
    static bool IsStateAcceptable(int itemState, bool acceptWorn, bool acceptDamaged, bool acceptBadlyDamaged)
    {
        switch (itemState)
        {
            case TraderXItemState.PRISTINE:
                return true;
            case TraderXItemState.WORN:
                return acceptWorn;
            case TraderXItemState.DAMAGED:
                return acceptDamaged;
            case TraderXItemState.BADLY_DAMAGED:
                return acceptBadlyDamaged;
            case TraderXItemState.RUINED:
                return false;
            default:
                return true;
        }
        
        return true; // Fallback return statement
    }
}
