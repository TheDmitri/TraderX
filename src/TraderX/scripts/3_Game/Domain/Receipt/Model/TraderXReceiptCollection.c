class TraderXReceiptCollection
{
    string playerUID;
    ref array<ref TraderXReceipt> receipts;

    void TraderXReceiptCollection(string uid = "")
    {
        playerUID = uid;
        receipts = new array<ref TraderXReceipt>();
    }

    void AddReceipt(TraderXReceipt receipt)
    {
        receipts.Insert(receipt);
    }

    TraderXReceipt GetReceiptById(string receiptId)
    {
        foreach (TraderXReceipt receipt : receipts)
        {
            if (receipt.receiptId == receiptId)
                return receipt;
        }
        return null;
    }

    int GetPendingCount()
    {
        int count = 0;
        foreach (TraderXReceipt receipt : receipts)
        {
            if (receipt.IsPending())
                count++;
        }
        return count;
    }

    void PurgeExpired()
    {
        foreach (TraderXReceipt receipt : receipts)
        {
            if (receipt.IsPending() && receipt.IsExpired())
                receipt.Expire();
        }
    }
}
