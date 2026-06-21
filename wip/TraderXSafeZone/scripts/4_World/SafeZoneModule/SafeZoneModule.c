[CF_RegisterModule(TraderXSafeZoneModule)]
class TraderXSafeZoneModule: CF_ModuleWorld
{
    ref TXSafeZoneSettings settings;

    ref SafeZoneHandler safeZoneHandler;

    override void OnInit()
    {
        super.OnInit(); 
        EnableMissionStart();
        EnableInvokeConnect();
    }

    void OnMissionStartHandler()
    {
        if(GetGame().IsServer())
        {
            settings = TXSafeZoneSettings.Load();
            if(settings && settings.cleanupTimer > 0)
                GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(PeriodicCleanup, settings.cleanupTimer * 1000, true);
        }
        else
        {
            settings = new TXSafeZoneSettings();
            safeZoneHandler = new SafeZoneHandler();
        }

        InitializeRPCs();
    }
    
    bool  IsInTXSafeZone(PlayerBase player)
    {
        if(!settings || !settings.locations)
            return false;
            
        bool isInAnyZone = false;
            
        foreach(TXSafeZoneLocation location : settings.locations)
        {
            if(!location || !location.isActive)
                continue;
                    
            vector playerPos = player.GetPosition();
            vector flatPlayerPos = Vector(playerPos[0], 0, playerPos[2]);
            vector flatZonePos = Vector(location.position[0], 0, location.position[2]);
            float distance = vector.Distance(flatPlayerPos, flatZonePos);
                
            if(distance <= location.radius)
            {
                isInAnyZone = true;
                break;
            }
        }

        return isInAnyZone;
    }

    override void OnMissionStart(Class sender, CF_EventArgs args)
    {
        super.OnMissionStart(sender, args);
        OnMissionStartHandler();
    }
     
    void InitializeRPCs()
    {
        if(GetGame().IsServer())
        {
            GetRPCManager().AddRPC("TraderXSafeZone", "GetSafeZoneStatus", this, SingleplayerExecutionType.Server );
            GetRPCManager().AddRPC("TraderXSafeZone", "EntitiesCleanUpRequest", this, SingleplayerExecutionType.Server );
        }
        else
        {
            GetRPCManager().AddRPC("TraderXSafeZone", "GetConfigFromServer", this, SingleplayerExecutionType.Client );
        }
    }

    void OnPlayerConnect(PlayerIdentity sender)
    {
        if(!settings){
            Print("SafeZoneModule: Settings not loaded yet, skipping config send");
            return;
        }
        
        PlayerBase player = GetPlayerByIdentity(sender);
        if(player && IsPlayerAdmin(sender))
            player.SetTXAdminStatus(true);
        
        Print("SafeZoneModule: Sending config to client");
        GetRPCManager().SendRPC("TraderXSafeZone", "GetConfigFromServer", new Param1<TXSafeZoneSettings>(settings), true, sender);
    }

    PlayerBase GetPlayerByIdentity(PlayerIdentity sender)
  	{
  		int	low	 =	0;
  		int	high =	0;
  		GetGame().GetPlayerNetworkIDByIdentityID( sender.GetPlayerId(), low, high );
  		return PlayerBase.Cast( GetGame().GetObjectByNetworkId(low, high ));
  	}

    bool IsPlayerAdmin(PlayerIdentity identity)
    {
        if(!settings || !settings.adminSteamUIDs || !identity)
            return false;
        
        string steamId = identity.GetPlainId();
        return settings.adminSteamUIDs.Find(steamId) != -1;
    }

    void GetSafeZoneStatus(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if(type != CallType.Server)
            return;

        Param1<bool> data;
        if(!ctx.Read(data))
            return;

        PlayerBase player = GetPlayerByIdentity(sender);
        if(!player)
            return;
        
        bool isInZoneClient = data.param1;
        bool isInZoneServer = IsInTXSafeZone(player);

        if(isInZoneClient != isInZoneServer)
            Print("[TraderXSafeZone] WARNING: Client/Server mismatch for " + sender.GetName() + " — client=" + isInZoneClient + " server=" + isInZoneServer + " (server wins)");
        
        player.SetTXSafeZoneStatus(isInZoneServer);
        
        if(isInZoneServer)
        {
            player.SetAllowDamage(false);
            player.SaveInfluenzaLevel();
            player.ClearInfluenza();
            
            if(player.IsRestrained())
            {
                player.SetRestrained(false);
                ItemBase itemInHands = ItemBase.Cast(player.GetItemInHands());
                if(itemInHands)
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
    
    void EntitiesCleanUpRequest(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if(type != CallType.Server)
            return;

        Param1<TXSafeZoneLocation> data;
        if(!ctx.Read(data))
            return;

        TXSafeZoneLocation location = data.param1;
        if(!location)
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
        if(!animal || !location || !location.allowedAnimals)
            return false;
            
        foreach(string allowedAnimal : location.allowedAnimals)
        {
            if(CF_String.EqualsIgnoreCase(allowedAnimal, animal.GetType()))
                return true;
        }

        return false;
    }

    void PeriodicCleanup()
    {
        if(!settings || !settings.locations)
            return;
        
        foreach(TXSafeZoneLocation location : settings.locations)
        {
            if(!location || !location.isActive)
                continue;
            
            array<Object> nearObjects = new array<Object>();
            GetGame().GetObjectsAtPosition(location.position, location.radius, nearObjects, null);
            
            foreach(Object obj : nearObjects)
            {
                if(!obj)
                    continue;
                
                string objType = obj.GetType();
                
                if(settings.objectsToDelete && settings.objectsToDelete.Find(objType) != -1)
                {
                    GetGame().ObjectDelete(obj);
                }
            }
        }
    }

    void GetConfigFromServer(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if(type != CallType.Client)
            return;

        Param1<TXSafeZoneSettings> data;
        if(!ctx.Read(data))
            return;

        settings = data.param1;

        if(!settings){
            Print("SafeZoneModule: Received null settings from server");
            return;
        }

        safeZoneHandler.Setup(settings);
    }

    override void OnInvokeConnect(Class sender, CF_EventArgs args)
    {
        super.OnInvokeConnect(sender, args);

        CF_EventPlayerArgs cArgs = CF_EventPlayerArgs.Cast(args);

        if(!IsMissionHost())
            return;
        
        if(!cArgs.Player || !cArgs.Identity)
            return;
        
        OnPlayerConnect(cArgs.Identity);
    }
}
