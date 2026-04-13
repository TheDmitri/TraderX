// Recursive vehicle/cargo snapshot for receipts.
// Mirrors GarageItemData structure for compatibility with existing restore logic.
class TraderXReceiptVehicleData
{
    string className;
    int quantity;
    float health;
    int magAmmoCount;
    int liquidType;
    int foodStage;
    float temperature;
    float wetness;
    float energy;
    string locationType;        // "ATTACHMENT" or "CARGO"
    int locationIndex;
    int locationRow;
    int locationCol;
    bool locationFlipped;
    ref array<ref TraderXReceiptVehicleData> children;

    void TraderXReceiptVehicleData()
    {
        children = new array<ref TraderXReceiptVehicleData>();
    }

    static TraderXReceiptVehicleData FromEntity(EntityAI entity, string locType = "", int locIdx = 0, int row = 0, int col = 0, bool flipped = false)
    {
        if (!entity)
            return null;

        TraderXReceiptVehicleData data = new TraderXReceiptVehicleData();
        data.className = entity.GetType();
        data.quantity = entity.GetQuantity();
        data.health = entity.GetHealth();
        data.locationType = locType;
        data.locationIndex = locIdx;
        data.locationRow = row;
        data.locationCol = col;
        data.locationFlipped = flipped;

        // Serialize attachments
        int attCount = entity.GetInventory().AttachmentCount();
        for (int i = 0; i < attCount; i++)
        {
            EntityAI att = entity.GetInventory().GetAttachmentFromIndex(i);
            if (att)
            {
                int slotId = entity.GetInventory().GetAttachmentSlotId(i);
                TraderXReceiptVehicleData child = TraderXReceiptVehicleData.FromEntity(att, "ATTACHMENT", slotId);
                if (child)
                    data.children.Insert(child);
            }
        }

        // Serialize cargo
        CargoBase cargo = entity.GetInventory().GetCargo();
        if (cargo)
        {
            for (int j = 0; j < cargo.GetItemCount(); j++)
            {
                EntityAI cargoItem = cargo.GetItem(j);
                if (cargoItem)
                {
                    int cRow, cCol;
                    bool cFlipped;
                    cargo.GetItemRowCol(j, cRow, cCol);
                    cFlipped = cargoItem.GetInventory().IsCargoFlipped();
                    TraderXReceiptVehicleData cargoChild = TraderXReceiptVehicleData.FromEntity(cargoItem, "CARGO", 0, cRow, cCol, cFlipped);
                    if (cargoChild)
                        data.children.Insert(cargoChild);
                }
            }
        }

        return data;
    }
}
