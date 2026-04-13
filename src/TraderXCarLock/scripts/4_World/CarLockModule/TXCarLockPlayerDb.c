class TXCarLockPlayerDb
{
    private bool _isAdmin;
    private string _steamId;
    private string _playerName;
    private ref set<string> _unlockedVehicles;

    void TXCarLockPlayerDb()
    {
        _unlockedVehicles = new set<string>();
    }

    string GetSteamId()
    {
        return _steamId;
    }

    int GetOwnerId()
    {
        return _steamId.Substring(8, 9).ToInt();
    }

    bool IsOwnerId(int id)
    {
        return GetOwnerId() == id;
    }

    bool HasUnlockedVehicle(string lockId)
    {
        return _unlockedVehicles.Find(lockId) != -1;
    }

    bool IsAdmin()
    {
        return _isAdmin;
    }

    void SetAdmin(bool state)
    {
        _isAdmin = state;
        Save();
    }

    void AddUnlockedVehicle(string lockId)
    {
        if (!lockId || lockId == "")
        {
            GetTXCarLockLogger().LogError("AddUnlockedVehicle called with empty lockId");
            return;
        }
        if (!HasUnlockedVehicle(lockId))
        {
            _unlockedVehicles.Insert(lockId);
            Save();
        }
    }

    void RemoveUnlockedVehicle(string lockId)
    {
        if (!lockId || lockId == "")
            return;

        int index = _unlockedVehicles.Find(lockId);
        if (index != -1)
        {
            _unlockedVehicles.Remove(index);
            Save();
        }
    }

    static void RemoveVehicleFromAllPlayers(string lockId)
    {
        if (!lockId || lockId == "")
            return;

        // Update all online players' in-memory instances
        array<Man> players = new array<Man>();
        GetGame().GetPlayers(players);
        foreach (Man man : players)
        {
            PlayerBase player = PlayerBase.Cast(man);
            if (player)
            {
                TXCarLockPlayerDb playerDb = player.GetCarLockDb();
                if (playerDb)
                {
                    playerDb.RemoveUnlockedVehicle(lockId);
                }
            }
        }

        // Update offline players' JSON files directly
        string dbDir = TX_CARLOCK_DB_DIR;
        string filename;
        FileAttr attr;

        FindFileHandle findHandle = FindFile(dbDir + "/*.json", filename, attr, FindFileFlags.ALL);
        if (findHandle)
        {
            ProcessOfflinePlayerFile(filename, lockId, players);

            while (FindNextFile(findHandle, filename, attr))
            {
                ProcessOfflinePlayerFile(filename, lockId, players);
            }
            CloseFindFile(findHandle);
        }

        GetTXCarLockLogger().LogInfo(string.Format("Removed vehicle %1 from all player databases", lockId));
    }

    private static void ProcessOfflinePlayerFile(string filename, string lockId, array<Man> onlinePlayers)
    {
        if (filename.Length() < 14)
            return;

        string steamId = filename.Substring(8, filename.Length() - 13);
        if (steamId == "")
            return;

        // Skip online players (already handled)
        foreach (Man man : onlinePlayers)
        {
            PlayerBase onlinePlayer = PlayerBase.Cast(man);
            if (onlinePlayer && onlinePlayer.GetIdentity() && onlinePlayer.GetIdentity().GetPlainId() == steamId)
            {
                return;
            }
        }

        // Player is offline, load and modify their DB
        TXCarLockPlayerDb playerDb = Load(steamId, "");
        if (playerDb)
        {
            playerDb.RemoveUnlockedVehicle(lockId);
        }
    }

    static void MakeDirectoriesIfNotExist()
    {
        if (!FileExist(TX_CARLOCK_DB_DIR))
        {
            MakeDirectory(TX_CARLOCK_DB_DIR);
        }
    }

    void Initialize(string steamId, string playerName)
    {
        _steamId = steamId;
        _playerName = playerName;
        Save();
    }

    void Save()
    {
        JsonFileLoader<TXCarLockPlayerDb>.JsonSaveFile(string.Format(TX_CARLOCK_DB_FILENAME, _steamId), this);
    }

    static ref TXCarLockPlayerDb Load(string steamId, string playerName)
    {
        TXCarLockPlayerDb carLockPlayerDb = new TXCarLockPlayerDb();

        MakeDirectoriesIfNotExist();

        string path = string.Format(TX_CARLOCK_DB_FILENAME, steamId);
        if (FileExist(path))
        {
            JsonFileLoader<TXCarLockPlayerDb>.JsonLoadFile(path, carLockPlayerDb);
            return carLockPlayerDb;
        }

        carLockPlayerDb.Initialize(steamId, playerName);
        return carLockPlayerDb;
    }
}
