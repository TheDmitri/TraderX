// Vehicle delivery method chosen at purchase time
class ETraderXDeliveryMethod
{
    static const int PARKING = 0;   // Spawn at trader parking spot (default)
    static const int GARAGE  = 1;   // Store directly in garage (requires garage provider)
    static const int RECEIPT = 2;   // Issue receipt paper item (requires receipt provider)
}
