class TraderXInsuranceRepository
{
    static void EnsureDirectory()
    {
        if (!FileExist(TRADERX_DB_DIR_SERVER))
            MakeDirectory(TRADERX_DB_DIR_SERVER);
        if (!FileExist(TRADERX_INSURANCE_DIR))
            MakeDirectory(TRADERX_INSURANCE_DIR);
    }

    static TraderXInsurancePolicyCollection Load(string playerUID)
    {
        EnsureDirectory();

        TraderXInsurancePolicyCollection collection = new TraderXInsurancePolicyCollection(playerUID);
        string filePath = string.Format(TRADERX_INSURANCE_FILE, playerUID);

        if (FileExist(filePath))
        {
            TraderXJsonLoader<TraderXInsurancePolicyCollection>.LoadFromFile(filePath, collection, false);
        }

        return collection;
    }

    static void Save(string playerUID, TraderXInsurancePolicyCollection collection)
    {
        EnsureDirectory();

        string filePath = string.Format(TRADERX_INSURANCE_FILE, playerUID);
        TraderXJsonLoader<TraderXInsurancePolicyCollection>.SaveToFile(filePath, collection);
    }

    // Scans all insurance files on disk for the restart vehicle scan
    static array<string> GetAllPlayerUIDs()
    {
        array<string> uids = new array<string>();

        EnsureDirectory();

        string fileName;
        FileAttr fileAttr;
        FindFileHandle handle = FindFile(TRADERX_INSURANCE_DIR + "insurance_*.json", fileName, fileAttr, FindFileFlags.ALL);

        if (!handle)
            return uids;

        if (fileName != string.Empty)
            uids.Insert(ExtractUIDFromFilename(fileName));

        while (FindNextFile(handle, fileName, fileAttr))
        {
            if (fileName != string.Empty)
                uids.Insert(ExtractUIDFromFilename(fileName));
        }

        CloseFindFile(handle);
        return uids;
    }

    private static string ExtractUIDFromFilename(string fileName)
    {
        // "insurance_76561198047475641.json" → "76561198047475641"
        string uid = fileName;
        uid.Replace("insurance_", "");
        uid.Replace(".json", "");
        return uid;
    }
}
