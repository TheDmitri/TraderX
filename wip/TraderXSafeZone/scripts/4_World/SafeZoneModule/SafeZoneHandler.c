class SafeZoneHandler
{
    PlayerBase player;

    private bool isInZone = false;
    private bool hasStarted = false;
    private bool onLeaving = false;
    private bool kickRequested = false;
    private float afkTimer = 0.0;
    private vector lastPlayerPosition = Vector(0, 10, 0);

    ref TXSafeZoneSettings settings;
    ref TXSafeZoneLocation selectedLocation;
    ref TraderXSafeZoneNotificationUI safeZoneNotificationUI;

    bool IsPlayerInside()
    {
        return isInZone;
    }

    bool HasStarted()
    {
        return hasStarted;
    }

    void Setup(TXSafeZoneSettings settings)
    {
        this.settings = settings;

        hasStarted = true;

        player = PlayerBase.Cast(GetGame().GetPlayer());
        if(!player)
            return;

        GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(Update, 1000, true);
        GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(RemoveUnwantedEntities, 5000, true);
        
        if(settings.enableAfkDisconnect)
            GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(CheckAFK, 20000, true);
    }

    void Update()
    {
        CheckTXSafeZoneLocation();
    }

    void CheckAFK()
    {
        if(!player || !isInZone || !settings.enableAfkDisconnect)
            return;
        
        if(kickRequested)
            return;
        
        if(afkTimer > settings.kickAfterDelay)
        {
            vector currentPos = player.GetPosition();
            float distance = vector.Distance(currentPos, lastPlayerPosition);
            
            if(distance < 1.0)
            {
                NotificationSystem.AddNotificationExtended(15, "AFK Warning", "You will be kicked for being AFK in 30 seconds", "TraderXSafeZone/datasets/shield.paa");
                kickRequested = true;
                GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(KickPlayer, 30000);
                return;
            }
            
            lastPlayerPosition = currentPos;
            afkTimer = 0.0;
        }
        else
        {
            afkTimer += 20.0;
        }
    }

    void KickPlayer()
    {
        if(!player)
            return;
        
        vector currentPos = player.GetPosition();
        float distance = vector.Distance(currentPos, lastPlayerPosition);
        
        if(distance < 1.0)
        {
            GetGame().DisconnectSessionForce();
        }
        
        lastPlayerPosition = Vector(0, 10, 0);
        kickRequested = false;
        afkTimer = 0.0;
    }

    void CheckTXSafeZoneLocation()
    {
        if(!settings || !settings.locations)
            return;

        if (onLeaving)
        {
            foreach(TXSafeZoneLocation location: settings.locations)
            {
                if (location && location.isActive && IsInRadius(location.position, location.radius))
                {
                    DisableOnLeaving();
                    selectedLocation = location;
                    isInZone = true;
                    return;
                }
            }
            return;
        }

        foreach(TXSafeZoneLocation loc: settings.locations)
        {
            if(loc && loc.isActive)
            {
                CheckTXSafeZoneLocation(loc);
            }
        }
    }

    string GetNotifTitle()
    {
        return TraderXSafeZoneTernary<string>.If(settings.notificationTitle != "", settings.notificationTitle, "SafeZone");
    }

    void OnEnterSafeZone()
    {
        DisableOnLeaving();
        isInZone = true;
        
        if(player)
            lastPlayerPosition = player.GetPosition();
        afkTimer = 0.0;

        string msg = TraderXSafeZoneTernary<string>.If(settings.msgOnEnteringZone != "", settings.msgOnEnteringZone, "You entered the safezone!");

        NotificationSystem.AddNotificationExtended(settings.notificationTimer, GetNotifTitle(), msg, "TraderXSafeZone/datasets/shield.paa");
        GetRPCManager().SendRPC("TraderXSafeZone", "GetSafeZoneStatus", new Param1<bool>(isInZone), true, null);
    }

    void DisableOnLeaving()
    {
        onLeaving = false;
        GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(EndOnLeaving);
        if (safeZoneNotificationUI)
        {
            safeZoneNotificationUI.Delete();
            safeZoneNotificationUI = null;
        }
    }

    void StartOnLeaving()
    {
        if (!player || !player.IsAlive())
            return;

        if (onLeaving)
            return;

        int countdown = TraderXSafeZoneTernary<int>.If(settings.leaveZoneCountdown <= 0, 30, settings.leaveZoneCountdown);

        onLeaving = true;

        safeZoneNotificationUI = new TraderXSafeZoneNotificationUI();
        string countdownMsg = TraderXSafeZoneTernary<string>.If(settings.msgOnLeavingZoneCountdown != "", settings.msgOnLeavingZoneCountdown, "");

        safeZoneNotificationUI.StartCountdown(countdown, countdownMsg, GetNotifTitle());

        GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(EndOnLeaving, countdown * 1000);
    }

    void EndOnLeaving()
    {
        onLeaving = false;
        if (safeZoneNotificationUI)
        {
            safeZoneNotificationUI.Delete();
            safeZoneNotificationUI = null;
        }
        OnExitSafeZone();
    }

    void OnExitSafeZone()
    {
        isInZone = false;
        selectedLocation = null;
        afkTimer = 0.0;
        lastPlayerPosition = Vector(0, 10, 0);
        kickRequested = false;
        
        string msg = TraderXSafeZoneTernary<string>.If(settings.msgOnLeavingZone != "", settings.msgOnLeavingZone, "You have left the safezone!");
        
        NotificationSystem.AddNotificationExtended(settings.notificationTimer, GetNotifTitle(), msg, "TraderXSafeZone/datasets/shield.paa");
        GetRPCManager().SendRPC("TraderXSafeZone", "GetSafeZoneStatus", new Param1<bool>(isInZone), true, null);
    }

    void CheckTXSafeZoneLocation(TXSafeZoneLocation location)
    {
        if(!location)
            return;
            
        bool IsCurrentlyInZone = IsInRadius(location.position, location.radius);

        if(!isInZone && IsCurrentlyInZone)
        {
            selectedLocation = location;
            OnEnterSafeZone();
        }
        else if(isInZone && IsCurrentlyInZone && selectedLocation != location)
        {
            selectedLocation = location;
        }
        else if(isInZone && !IsCurrentlyInZone)
        {
            if(selectedLocation == location)
            {
                bool stillInAnyZone = false;
                foreach(TXSafeZoneLocation loc: settings.locations)
                {
                    if(loc && loc.isActive && loc != location && IsInRadius(loc.position, loc.radius))
                    {
                        stillInAnyZone = true;
                        selectedLocation = loc;
                        break;
                    }
                }
                
                if(!stillInAnyZone)
                {
                    StartOnLeaving();
                }
            }
        }
    }

    bool IsInRadius(vector position, int radius)
    {
        if(!player)
            return false;
            
        vector playerPos = Vector(player.GetPosition()[0], 0, player.GetPosition()[2]);
        vector safeZonePos = Vector(position[0], 0, position[2]);
        float distance = vector.Distance(playerPos, safeZonePos);
        return distance <= radius;
    }   

    void RemoveUnwantedEntities()
    {
        if (!selectedLocation || !selectedLocation.isEntitiesCleanActive)
            return;

        array<Object> safeZoneCleanUpList = new array<Object>();
        GetGame().GetObjectsAtPosition(selectedLocation.position, selectedLocation.radius, safeZoneCleanUpList, null);

        bool needsCleanup = false;
        foreach (Object obj : safeZoneCleanUpList)
        {
            if (obj.IsInherited(ZombieBase) || (obj.IsInherited(AnimalBase) && !IsAllowedAnimal(obj, selectedLocation)))
            {
                needsCleanup = true;
                break;
            }
        }

        if (needsCleanup)
            GetRPCManager().SendRPC("TraderXSafeZone", "EntitiesCleanUpRequest", new Param1<TXSafeZoneLocation>(selectedLocation), true, null);
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
}
