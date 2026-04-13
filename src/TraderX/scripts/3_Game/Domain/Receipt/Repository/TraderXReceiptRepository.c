class TraderXReceiptRepository
{
    static void EnsureDirectory()
    {
        if (!FileExist(TRADERX_DB_DIR_SERVER))
            MakeDirectory(TRADERX_DB_DIR_SERVER);
        if (!FileExist(TRADERX_RECEIPTS_DIR))
            MakeDirectory(TRADERX_RECEIPTS_DIR);
    }

    static TraderXReceiptCollection Load(string playerUID)
    {
        EnsureDirectory();

        TraderXReceiptCollection collection = new TraderXReceiptCollection(playerUID);
        string filePath = string.Format(TRADERX_RECEIPTS_FILE, playerUID);

        if (FileExist(filePath))
        {
            TraderXJsonLoader<TraderXReceiptCollection>.LoadFromFile(filePath, collection, false);
        }

        return collection;
    }

    static void Save(string playerUID, TraderXReceiptCollection collection)
    {
        EnsureDirectory();

        string filePath = string.Format(TRADERX_RECEIPTS_FILE, playerUID);
        TraderXJsonLoader<TraderXReceiptCollection>.SaveToFile(filePath, collection);
    }
}
