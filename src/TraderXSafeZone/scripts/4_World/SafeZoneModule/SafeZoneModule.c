class TraderXSafeZoneModule : DayZularService
{
    ref TXSafeZoneSettings settings;

    ref SafeZoneHandler safeZoneHandler;

    void TraderXSafeZoneModule()
    {
        GetDayZularEvents().OnMissionStart.Insert(OnMissionStart);
        GetDayZularEvents().OnPlayerJoined.Insert(OnPlayerJoined);
    }

    void OnMissionStart()
    {
        OnMissionStartHandler();
    }

    void OnMissionStartHandler()
    {
        if (GetGame().IsServer())
        {
            settings = TXSafeZoneSettings.Load();
            if (settings && settings.cleanupTimer > 0)
                GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(PeriodicCleanup, settings.cleanupTimer * 1000, true);
        }
        else
        {
            settings = new TXSafeZoneSettings();
            safeZoneHandler = new SafeZoneHandler();
        }

        InitializeRPCs();
    }

    bool IsInTXSafeZone(PlayerBase player)
    {
        if (!settings || !settings.locations)
            return false;

        bool isInAnyZone = false;

        foreach (TXSafeZoneLocation location : settings.locations)
        {
            if (!location || !location.isActive)
                continue;

            vector playerPos = player.GetPosition();
            vector flatPlayerPos = Vector(playerPos[0], 0, playerPos[2]);
            vector flatZonePos = Vector(location.position[0], 0, location.position[2]);
            float distance = vector.Distance(flatPlayerPos, flatZonePos);

            if (distance <= location.radius)
            {
                isInAnyZone = true;
                break;
            }
        }

        return isInAnyZone;
    }

    void InitializeRPCs()
    {
        if (GetGame().IsServer())
        {
            GetDayZularRPCManager().AddRPC("TraderXSafeZone", "GetSafeZoneStatus", this, DayZularRPCType.Server);
            GetDayZularRPCManager().AddRPC("TraderXSafeZone", "EntitiesCleanUpRequest", this, DayZularRPCType.Server);
        }
        else
        {
            GetDayZularRPCManager().AddRPC("TraderXSafeZone", "GetConfigFromServer", this, DayZularRPCType.Client);
        }
    }

    void OnPlayerJoined(PlayerBase player, PlayerIdentity identity)
    {
        if (!GetGame().IsServer())
            return;

        OnPlayerConnect(identity);
    }

    void OnPlayerConnect(PlayerIdentity sender)
    {
        if (!settings)
        {
            Print("SafeZoneModule: Settings not loaded yet, skipping config send");
            return;
        }

        PlayerBase player = GetPlayerByIdentity(sender);
        if (player && IsPlayerAdmin(sender))
            player.SetTXAdminStatus(true);

        Print("SafeZoneModule: Sending config to client");
        GetDayZularRPCManager().SendRPC("TraderXSafeZone", "GetConfigFromServer", new Param1<TXSafeZoneSettings>(settings), true, sender);
    }

    PlayerBase GetPlayerByIdentity(PlayerIdentity sender)
    {
        int low = 0;
        int high = 0;
        GetGame().GetPlayerNetworkIDByIdentityID(sender.GetPlayerId(), low, high);
        return PlayerBase.Cast(GetGame().GetObjectByNetworkId(low, high));
    }

    bool IsPlayerAdmin(PlayerIdentity identity)
    {
        if (!settings || !settings.adminSteamUIDs || !identity)
            return false;

        string steamId = identity.GetPlainId();
        return settings.adminSteamUIDs.Find(steamId) != -1;
    }

    void GetSafeZoneStatus(PlayerIdentity sender, ParamsReadContext ctx)
    {
        Param1<bool> data;
        if (!ctx.Read(data))
            return;

        PlayerBase player = GetPlayerByIdentity(sender);
        if (!player)
            return;

        bool isInZoneClient = data.param1;
        bool isInZoneServer = IsInTXSafeZone(player);

        if (isInZoneClient != isInZoneServer)
            Print("[TraderXSafeZone] WARNING: Client/Server mismatch for " + sender.GetName() + " — client=" + isInZoneClient + " server=" + isInZoneServer + " (server wins)");

        player.SetTXSafeZoneStatus(isInZoneServer);

        if (isInZoneServer)
        {
            player.SetAllowDamage(false);
            player.SaveInfluenzaLevel();
            player.ClearInfluenza();

            if (player.IsRestrained())
            {
                player.SetRestrained(false);
                ItemBase itemInHands = ItemBase.Cast(player.GetItemInHands());
                if (itemInHands)
                {
                    itemInHands.AddHealth(-50);
                    MiscGameplayFunctions.TransformRestrainItem(itemInHands, null, null, player);
                }
            }
        }
        else
        {
            player.SetAllowDamage(true);
            player.RestoreInfluenza();
        }
    }

    void EntitiesCleanUpRequest(PlayerIdentity sender, ParamsReadContext ctx)
    {
        Param1<TXSafeZoneLocation> data;
        if (!ctx.Read(data))
            return;

        TXSafeZoneLocation location = data.param1;
        if (!location)
            return;

        array<Object> safeZoneCleanUpList = new array<Object>();
        GetGame().GetObjectsAtPosition(location.position, location.radius, safeZoneCleanUpList, null);

        foreach (Object obj : safeZoneCleanUpList)
        {
            if (IsInRadius(obj, location) && (obj.IsInherited(ZombieBase) || (obj.IsInherited(AnimalBase) && !IsAllowedAnimal(obj, location))))
            {
                GetGame().ObjectDelete(obj);
            }
        }
    }

    bool IsInRadius(Object obj, TXSafeZoneLocation location)
    {
        vector objectPos = obj.GetPosition();
        vector flatObjectPos = Vector(objectPos[0], 0, objectPos[2]);
        vector flatZonePos = Vector(location.position[0], 0, location.position[2]);
        float distance = vector.Distance(flatObjectPos, flatZonePos);
        return distance <= location.radius;
    }

    bool IsAllowedAnimal(Object animal, TXSafeZoneLocation location)
    {
        if (!animal || !location || !location.allowedAnimals)
            return false;

        foreach (string allowedAnimal : location.allowedAnimals)
        {
            if (DZlrString.EqualsIgnoreCase(allowedAnimal, animal.GetType()))
                return true;
        }

        return false;
    }

    void PeriodicCleanup()
    {
        if (!settings || !settings.locations)
            return;

        foreach (TXSafeZoneLocation location : settings.locations)
        {
            if (!location || !location.isActive)
                continue;

            array<Object> nearObjects = new array<Object>();
            GetGame().GetObjectsAtPosition(location.position, location.radius, nearObjects, null);

            foreach (Object obj : nearObjects)
            {
                if (!obj)
                    continue;

                string objType = obj.GetType();

                if (settings.objectsToDelete && settings.objectsToDelete.Find(objType) != -1)
                {
                    GetGame().ObjectDelete(obj);
                }
            }
        }
    }

    void GetConfigFromServer(PlayerIdentity sender, ParamsReadContext ctx)
    {
        Param1<TXSafeZoneSettings> data;
        if (!ctx.Read(data))
            return;

        settings = data.param1;

        if (!settings)
        {
            Print("SafeZoneModule: Received null settings from server");
            return;
        }

        safeZoneHandler.Setup(settings);
    }
}
