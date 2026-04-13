class ETraderXReceiptStatus
{
    static const string PENDING  = "PENDING";
    static const string REDEEMED = "REDEEMED";
    static const string EXPIRED  = "EXPIRED";
}

class TraderXReceipt
{
    string receiptId;
    string playerUID;
    string vehicleType;
    string presetId;
    ref array<string> presetAttachments;
    int buyPrice;
    ref array<int> lockData;
    int createdTimestamp;
    int expiryTimestamp;
    string status;

    void TraderXReceipt()
    {
        presetAttachments = new array<string>();
        lockData = new array<int>();
        status = ETraderXReceiptStatus.PENDING;
    }

    static TraderXReceipt Create(string playerUID, string vehicleType, string presetId, array<string> attachments, int price, array<int> lock, int expiryDays)
    {
        TraderXReceipt receipt = new TraderXReceipt();
        receipt.receiptId = UUID.Generate();
        receipt.playerUID = playerUID;
        receipt.vehicleType = vehicleType;
        receipt.presetId = presetId;
        receipt.buyPrice = price;
        receipt.createdTimestamp = GetGame().GetTime() / 1000;

        if (expiryDays > 0)
            receipt.expiryTimestamp = receipt.createdTimestamp + (expiryDays * 86400);
        else
            receipt.expiryTimestamp = 0;

        if (attachments)
        {
            foreach (string att : attachments)
            {
                receipt.presetAttachments.Insert(att);
            }
        }

        if (lock)
        {
            foreach (int lockVal : lock)
            {
                receipt.lockData.Insert(lockVal);
            }
        }

        return receipt;
    }

    bool IsPending()
    {
        return status == ETraderXReceiptStatus.PENDING;
    }

    bool IsExpired()
    {
        if (expiryTimestamp <= 0)
            return false;

        int now = GetGame().GetTime() / 1000;
        return now > expiryTimestamp;
    }

    void Redeem()
    {
        status = ETraderXReceiptStatus.REDEEMED;
    }

    void Expire()
    {
        status = ETraderXReceiptStatus.EXPIRED;
    }

    string GetDisplayName()
    {
        return vehicleType;
    }
}
