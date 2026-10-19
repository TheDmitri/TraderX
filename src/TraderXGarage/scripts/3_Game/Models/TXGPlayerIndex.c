// TraderXGarage - Player Index (1.30 FindFile-free registry)
// File: $profile:TraderX/TraderXGarage/Database/PlayerIndex.json

class TXGPlayerIndexEntry
{
    string uid;
    string name;

    void TXGPlayerIndexEntry(string _uid = "", string _name = "")
    {
        uid = _uid;
        name = _name;
    }
}

class TXGPlayerIndex
{
    string version = TXGConstants.DATA_VERSION;
    ref array<ref TXGPlayerIndexEntry> entries;

    protected static ref TXGPlayerIndex s_Instance;

    void TXGPlayerIndex()
    {
        entries = new array<ref TXGPlayerIndexEntry>();
    }

    static TXGPlayerIndex Get()
    {
        if (!s_Instance)
            s_Instance = Load();
        return s_Instance;
    }

    static TXGPlayerIndex Load()
    {
        if (!FileExist(TXGConstants.ROOT_DIR))
            MakeDirectory(TXGConstants.ROOT_DIR);
        if (!FileExist(TXGConstants.CONFIG_DIR))
            MakeDirectory(TXGConstants.CONFIG_DIR);
        if (!FileExist(TXGConstants.DB_DIR))
            MakeDirectory(TXGConstants.DB_DIR);

        TXGPlayerIndex idx = new TXGPlayerIndex();
        if (FileExist(TXGConstants.PLAYER_INDEX_FILE))
        {
            JsonFileLoader<TXGPlayerIndex>.JsonLoadFile(TXGConstants.PLAYER_INDEX_FILE, idx);
            if (!idx.entries)
                idx.entries = new array<ref TXGPlayerIndexEntry>();
        }
        else
        {
            idx.SeedFromExistingFiles();
            idx.Save();
        }
        return idx;
    }

    protected void SeedFromExistingFiles()
    {
        string fileName;
        FileAttr fileAttr;
        FindFileHandle handle = TXGFileSystem.Find(TXGConstants.DB_DIR + "Garage_*.json", fileName, fileAttr, FindFileFlags.ALL);
        if (!handle)
            return;

        TrySeedFile(fileName);
        while (FindNextFile(handle, fileName, fileAttr))
        {
            TrySeedFile(fileName);
        }
        CloseFindFile(handle);
    }

    protected void TrySeedFile(string fileName)
    {
        if (fileName.IndexOf("Garage_") != 0 || fileName.Length() <= 12)
            return;
        string uid = fileName.Substring(7, fileName.Length() - 12);
        if (uid != "")
            entries.Insert(new TXGPlayerIndexEntry(uid, ""));
    }

    void Save()
    {
        if (!FileExist(TXGConstants.DB_DIR))
            MakeDirectory(TXGConstants.DB_DIR);
        JsonFileLoader<TXGPlayerIndex>.JsonSaveFile(TXGConstants.PLAYER_INDEX_FILE, this);
    }

    void Touch(string uid, string name)
    {
        if (uid == "")
            return;

        foreach (TXGPlayerIndexEntry e : entries)
        {
            if (e && e.uid == uid)
            {
                if (name != "" && e.name != name)
                {
                    e.name = name;
                    Save();
                }
                return;
            }
        }

        entries.Insert(new TXGPlayerIndexEntry(uid, name));
        Save();
    }
}
