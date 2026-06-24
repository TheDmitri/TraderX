// TraderXGarage - Recursive item snapshot
// Captures a vehicle's attachments + cargo tree for faithful restore.
// Persisted as nested array<ref> (no map<>) so JsonFileLoader round-trips it.

class TXGItemSnapshot
{
    string item;            // class name
    int    quantity;
    int    slot;            // attachment slot id (when isAttachment)
    int    magAmmoCount;
    int    liquidType;
    int    foodstage;
    int    extraData;
    float  temp;
    float  wetness;
    float  health;          // global health of the item
    float  energy;
    bool   isAttachment;    // true = attachment, false = cargo item

    ref array<ref TXGItemSnapshot> childrens;

    void TXGItemSnapshot()
    {
        childrens = new array<ref TXGItemSnapshot>();
    }
}
