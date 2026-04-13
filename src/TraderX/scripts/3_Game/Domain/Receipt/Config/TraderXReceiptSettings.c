class TraderXReceiptSettings
{
    bool enabled = true;
    int receiptExpiryDays = 7;          // 0 = no expiry
    bool saveLockData = true;           // save car lock state in receipt
    bool saveAttachments = true;        // save preset attachments
    int maxReceiptsPerPlayer = 10;      // max pending receipts

    static TraderXReceiptSettings Load()
    {
        TraderXReceiptSettings settings = new TraderXReceiptSettings();

        if (FileExist(TRADERX_RECEIPT_CONFIG_FILE))
        {
            JsonFileLoader<TraderXReceiptSettings>.JsonLoadFile(TRADERX_RECEIPT_CONFIG_FILE, settings);
            return settings;
        }

        settings.Save();
        return settings;
    }

    void Save()
    {
        JsonFileLoader<TraderXReceiptSettings>.JsonSaveFile(TRADERX_RECEIPT_CONFIG_FILE, this);
    }
}
