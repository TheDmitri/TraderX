/**
 * Physical receipt paper item that players carry.
 * The vehicle data is NOT stored in this item — only a receiptId
 * that references the server-side JSON record.
 *
 * Duplicating this item is harmless: the first redeem marks the
 * DB record as REDEEMED, and all subsequent attempts fail.
 */
class TraderXReceiptItem extends ItemBase
{
    // Synced to client for tooltip display
    protected string m_ReceiptId;
    protected string m_VehicleType;

    void TraderXReceiptItem()
    {
        m_ReceiptId = "";
        m_VehicleType = "";

        RegisterNetSyncVariableString("m_ReceiptId");
        RegisterNetSyncVariableString("m_VehicleType");
    }

    void SetReceiptId(string receiptId)
    {
        m_ReceiptId = receiptId;
        SetSynchDirty();
    }

    string GetReceiptId()
    {
        return m_ReceiptId;
    }

    void SetVehicleType(string vehicleType)
    {
        m_VehicleType = vehicleType;
        SetSynchDirty();
    }

    string GetVehicleType()
    {
        return m_VehicleType;
    }

    override bool CanPutInCargo(EntityAI parent)
    {
        return true;
    }

    override bool CanPutIntoHands(EntityAI parent)
    {
        return true;
    }

    // Display info in item tooltip
    override void SetActions()
    {
        super.SetActions();
        // Deploy action will be added here when action system is configured
        // AddAction(ActionDeployTraderXReceipt);
    }

    override bool NameOverride(out string output)
    {
        if (m_VehicleType != "")
        {
            output = "Vehicle Receipt: " + m_VehicleType;
            return true;
        }
        return false;
    }

    override bool DescriptionOverride(out string output)
    {
        if (m_ReceiptId != "")
        {
            output = "Receipt for " + m_VehicleType + "\nID: " + m_ReceiptId + "\nDeploy near a trader to receive your vehicle.";
            return true;
        }
        return false;
    }
}
