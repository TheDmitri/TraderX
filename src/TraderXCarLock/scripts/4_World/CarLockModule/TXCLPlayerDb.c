#ifndef CarLock
// TraderPlus V1 defines "CarLock" and owns the CarLock item: TraderX CarLock stands down when it is loaded (see TXCLTraderPlusNotice).
// Per-player persistence: which locks they have unlocked + active wrong-code cooldowns.
// JSON at $profile:TraderX\TraderXCarLock\Database\<steamId>.json.
// JsonFileLoader can't round-trip map<> (see memory dayz-jsonfileloader-no-map), so the
// unlocked set and cooldowns are stored as set/array of refs.

class TXCLPlayerDb
{
    private bool _isAdmin;
    private string _steamId;
    private string _playerName;
    private ref set<string> _unlockedVehicles;
    private ref array<ref TXCLCooldownState> _cooldowns;

    void TXCLPlayerDb()
    {
        _unlockedVehicles = new set<string>();
        _cooldowns = new array<ref TXCLCooldownState>();
    }

    // TheLab-compatible owner id: digits 9..17 of the steam64 string (or already-converted low id).
    static int SteamIdToOwnerId(string steamId)
    {
        if (steamId.Length() >= 17)
            return steamId.Substring(8, 9).ToInt();
        if (steamId.Length() > 0 && steamId.Length() <= 10)
        {
            int directId = steamId.ToInt();
            if (directId > 0)
                return directId;
        }
        return TXCL_UNDEFINED;
    }

    string GetSteamId()
    {
        return _steamId;
    }

    int GetOwnerId()
    {
        return SteamIdToOwnerId(_steamId);
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

    TXCLCooldownState GetCooldownStateById(string lockId)
    {
        foreach (TXCLCooldownState cooldownState : _cooldowns)
        {
            if (cooldownState._carlockId == lockId)
                return cooldownState;
        }
        return null;
    }

    void AddCooldownState(TXCLCooldownState cooldownState)
    {
        CleanupExpiredCooldowns();
        _cooldowns.Insert(cooldownState);
        Save();
    }

    void CleanupExpiredCooldowns()
    {
        for (int i = _cooldowns.Count() - 1; i >= 0; i--)
        {
            if (_cooldowns[i] && !_cooldowns[i].HasCooldown())
                _cooldowns.Remove(i);
        }
    }

    void AddUnlockedVehicle(string lockId)
    {
        if (!lockId || lockId == "")
            return;
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

    // Drop a lock from every player's DB (online + offline) when the lock is removed.
    static void RemoveVehicleFromAllPlayers(string lockId)
    {
        if (!lockId || lockId == "")
            return;

        array<Man> players = new array<Man>();
        GetGame().GetPlayers(players);
        foreach (Man man : players)
        {
            PlayerBase player = PlayerBase.Cast(man);
            if (player)
            {
                TXCLPlayerDb playerDb = player.GetTXCLPlayerDb();
                if (playerDb)
                    playerDb.RemoveUnlockedVehicle(lockId);
            }
        }

        string filename;
        FileAttr attr;
        FindFileHandle findHandle = TXCLFileSystem.Find(TXCL_DB_DIR + "*.json", filename, attr, FindFileFlags.ALL);
        if (findHandle)
        {
            ProcessOfflinePlayerFile(filename, lockId, players);
            while (FindNextFile(findHandle, filename, attr))
                ProcessOfflinePlayerFile(filename, lockId, players);
            CloseFindFile(findHandle);
        }
    }

    private static void ProcessOfflinePlayerFile(string filename, string lockId, array<Man> onlinePlayers)
    {
        // Filename is "<steamId>.json" -> strip the 5-char ".json" suffix.
        if (filename.Length() < 6)
            return;

        string steamId = filename.Substring(0, filename.Length() - 5);
        if (steamId == "")
            return;

        foreach (Man man : onlinePlayers)
        {
            PlayerBase onlinePlayer = PlayerBase.Cast(man);
            if (onlinePlayer && onlinePlayer.GetIdentity() && onlinePlayer.GetIdentity().GetPlainId() == steamId)
                return;
        }

        TXCLPlayerDb playerDb = Load(steamId, "");
        if (playerDb)
            playerDb.RemoveUnlockedVehicle(lockId);
    }

    static void MakeDirectoriesIfNotExist()
    {
        if (!FileExist(TXCL_PROFILE_ROOT))
            MakeDirectory(TXCL_PROFILE_ROOT);
        if (!FileExist(TXCL_CONFIG_ROOT))
            MakeDirectory(TXCL_CONFIG_ROOT);
        if (!FileExist(TXCL_DB_DIR))
            MakeDirectory(TXCL_DB_DIR);
    }

    void Initialize(string steamId, string playerName)
    {
        _steamId = steamId;
        _playerName = playerName;
        Save();
    }

    void Save()
    {
        JsonFileLoader<TXCLPlayerDb>.JsonSaveFile(string.Format(TXCL_DB_FILENAME, _steamId), this);
    }

    static ref TXCLPlayerDb Load(string steamId, string playerName)
    {
        TXCLPlayerDb carLockPlayerDb = new TXCLPlayerDb();

        MakeDirectoriesIfNotExist();

        string path = string.Format(TXCL_DB_FILENAME, steamId);
        if (FileExist(path))
        {
            JsonFileLoader<TXCLPlayerDb>.JsonLoadFile(path, carLockPlayerDb);
            carLockPlayerDb.CleanupExpiredCooldownsOnLoad();
            return carLockPlayerDb;
        }

        carLockPlayerDb.Initialize(steamId, playerName);
        return carLockPlayerDb;
    }

    void CleanupExpiredCooldownsOnLoad()
    {
        int currentTime = CF_Date.Now().GetTimestamp();
        int removedCount = 0;

        for (int i = _cooldowns.Count() - 1; i >= 0; i--)
        {
            TXCLCooldownState cooldown = _cooldowns[i];
            if (!cooldown)
            {
                _cooldowns.Remove(i);
                removedCount++;
                continue;
            }

            if (cooldown._timestamp <= currentTime)
            {
                _cooldowns.Remove(i);
                removedCount++;
            }
        }

        if (removedCount > 0)
            Save();
    }
}
#endif
