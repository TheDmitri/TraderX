// TraderXGarage - RPC Service
// Client <-> server communication via the DayZular RPC Manager.

class TXGRPCService extends DayZularService
{
    void TXGRPCService()
    {
        GetDayZularEvents().OnPlayerJoined.Insert(OnPlayerJoined);
        RegisterRPCs();
    }

    protected void RegisterRPCs()
    {
        if (GetGame().IsServer() || !GetGame().IsMultiplayer())
        {
            GetDayZularRPCManager().AddRPC(TXGConstants.RPC_NAMESPACE, "RequestGarage", this, DayZularRPCType.Server);
            GetDayZularRPCManager().AddRPC(TXGConstants.RPC_NAMESPACE, "RequestParkIn", this, DayZularRPCType.Server);
            GetDayZularRPCManager().AddRPC(TXGConstants.RPC_NAMESPACE, "RequestParkOut", this, DayZularRPCType.Server);
            GetDayZularRPCManager().AddRPC(TXGConstants.RPC_NAMESPACE, "RequestRepair", this, DayZularRPCType.Server);
            GetDayZularRPCManager().AddRPC(TXGConstants.RPC_NAMESPACE, "RequestInsure", this, DayZularRPCType.Server);
            GetDayZularRPCManager().AddRPC(TXGConstants.RPC_NAMESPACE, "RequestShare", this, DayZularRPCType.Server);
            GetDayZularRPCManager().AddRPC(TXGConstants.RPC_NAMESPACE, "RequestAdminImpound", this, DayZularRPCType.Server);

            GetDayZularRPCManager().SetRateLimit(TXGConstants.RPC_NAMESPACE, "RequestGarage", 4, 1.0);
            GetDayZularRPCManager().SetRateLimit(TXGConstants.RPC_NAMESPACE, "RequestParkIn", 3, 1.0);
            GetDayZularRPCManager().SetRateLimit(TXGConstants.RPC_NAMESPACE, "RequestParkOut", 3, 1.0);
            GetDayZularRPCManager().SetRateLimit(TXGConstants.RPC_NAMESPACE, "RequestRepair", 3, 1.0);
            GetDayZularRPCManager().SetRateLimit(TXGConstants.RPC_NAMESPACE, "RequestInsure", 3, 1.0);
            GetDayZularRPCManager().SetRateLimit(TXGConstants.RPC_NAMESPACE, "RequestShare", 3, 1.0);
            GetDayZularRPCManager().SetRateLimit(TXGConstants.RPC_NAMESPACE, "RequestAdminImpound", 3, 1.0);
        }

        if (GetGame().IsClient() || !GetGame().IsMultiplayer())
        {
            GetDayZularRPCManager().AddRPC(TXGConstants.RPC_NAMESPACE, "OnGarageList", this, DayZularRPCType.Client);
            GetDayZularRPCManager().AddRPC(TXGConstants.RPC_NAMESPACE, "OnGarageConfig", this, DayZularRPCType.Client);
            GetDayZularRPCManager().AddRPC(TXGConstants.RPC_NAMESPACE, "OnGarageNotify", this, DayZularRPCType.Client);
        }
    }

    // --- Server: on join, push the lightweight config (costs, slots, npc classes) ---

    void OnPlayerJoined(PlayerBase player, PlayerIdentity identity)
    {
        if (!GetGame().IsServer() || !identity)
            return;

        TXGSettings settings = GetTXGConfigService().GetServerSettings();
        if (!settings)
            return;

        TXGConfigDTO dto = new TXGConfigDTO();
        dto.parkInCost = settings.parkInCost;
        dto.parkOutCost = settings.parkOutCost;
        dto.maxSlots = settings.maxVehicleStored;
        dto.enableRepairs = settings.enableRepairs;
        if (settings.insurance)
        {
            dto.enableInsurance = settings.insurance.enabled;
            dto.insuranceDefaultDays = settings.insurance.minDays;
            dto.insuranceMaxDays = settings.insurance.maxDays;
        }
        dto.isAdmin = GetTXGGarageService().IsPlayerAdmin(identity.GetPlainId());
        foreach (TXGNpcEntry npc : settings.npcs)
        {
            if (!npc)
                continue;
            if (npc.className != "")
                dto.npcClasses.Insert(npc.className);
            if (npc.npcId >= 0)
                dto.npcIds.Insert(npc.npcId);
            if (npc.parkSpots)
            {
                foreach (TXGParkSpot sp : npc.parkSpots)
                {
                    if (sp)
                        dto.npcSpots.Insert(new TXGNpcSpotDTO(npc.npcId, npc.position, sp.position, sp.orientation, sp.size));
                }
            }
        }

        GetDayZularRPCManager().SendRPC(TXGConstants.RPC_NAMESPACE, "OnGarageConfig", new Param1<ref TXGConfigDTO>(dto), true, identity);
        NotifyPendingHolds(identity);
    }

    // Insurance replacements are created while the owner may be offline: tell them at login.
    protected void NotifyPendingHolds(PlayerIdentity identity)
    {
        string uid = identity.GetPlainId();
        if (!FileExist(TXGGarageData.GetFilePath(uid)))
            return;

        TXGGarageData data = TXGGarageData.Load(uid, identity.GetName());
        int holds = 0;
        foreach (TXGVehicleData v : data.vehicles)
        {
            if (v && v.holdReason == TXGConstants.HOLD_INSURANCE)
                holds++;
        }
        if (holds > 0)
            NotificationSystem.SendNotificationToPlayerIdentityExtended(identity, 10, "Garage", holds.ToString() + " insurance replacement vehicle(s) waiting at the garage.", "set:dayz_gui image:icon_info");
    }

    // --- Server handlers ---
    // Player position is read authoritatively server-side (never trust client coords),
    // which also avoids shipping uninstantiated Param<vector> RPC combos.

    void RequestGarage(PlayerIdentity sender, ParamsReadContext ctx)
    {
        PlayerBase player = GetPlayerByIdentity(sender);
        if (!player)
            return;

        SendGarageList(player);
    }

    void RequestParkIn(PlayerIdentity sender, ParamsReadContext ctx)
    {
        PlayerBase player = GetPlayerByIdentity(sender);
        if (!player)
            return;

        bool ok = false;
        string msg = GetTXGGarageService().ParkIn(player, player.GetPosition(), ok);
        Notify(player, msg, ok);
        SendGarageList(player);
    }

    void RequestParkOut(PlayerIdentity sender, ParamsReadContext ctx)
    {
        Param1<ref TXGParkOutRequest> data = new Param1<ref TXGParkOutRequest>(null);
        if (!ctx.Read(data) || !data.param1)
            return;

        PlayerBase player = GetPlayerByIdentity(sender);
        if (!player)
            return;

        TXGParkOutRequest req = data.param1;
        string status;
        EntityAI vehicle = GetTXGGarageService().ParkOut(player, req.vehicleId, req.npcId, player.GetPosition(), status);
        Notify(player, status, vehicle != null);
        SendGarageList(player);
    }

    void RequestRepair(PlayerIdentity sender, ParamsReadContext ctx)
    {
        Param1<ref TXGVehicleActionRequest> data = new Param1<ref TXGVehicleActionRequest>(null);
        if (!ctx.Read(data) || !data.param1)
            return;

        PlayerBase player = GetPlayerByIdentity(sender);
        if (!player)
            return;

        bool ok = false;
        string msg = GetTXGGarageService().RepairStoredVehicle(player, data.param1.vehicleId, ok);
        Notify(player, msg, ok);
        SendGarageList(player);
    }

    void RequestInsure(PlayerIdentity sender, ParamsReadContext ctx)
    {
        Param1<ref TXGVehicleActionRequest> data = new Param1<ref TXGVehicleActionRequest>(null);
        if (!ctx.Read(data) || !data.param1)
            return;

        PlayerBase player = GetPlayerByIdentity(sender);
        if (!player)
            return;

        bool ok = false;
        int days = data.param1.intParam;
        if (days <= 0)
            days = 1;
        string msg = GetTXGInsuranceService().SubscribeStoredVehicle(player, data.param1.vehicleId, days, ok);
        Notify(player, msg, ok);
        SendGarageList(player);
    }

    void RequestShare(PlayerIdentity sender, ParamsReadContext ctx)
    {
        Param1<ref TXGVehicleActionRequest> data = new Param1<ref TXGVehicleActionRequest>(null);
        if (!ctx.Read(data) || !data.param1)
            return;

        PlayerBase player = GetPlayerByIdentity(sender);
        if (!player)
            return;

        bool ok = false;
        string msg = GetTXGGarageService().ShareStoredVehicle(player, data.param1.vehicleId, data.param1.strParam, data.param1.intParam, ok);
        Notify(player, msg, ok);
        SendGarageList(player);
    }

    void RequestAdminImpound(PlayerIdentity sender, ParamsReadContext ctx)
    {
        PlayerBase player = GetPlayerByIdentity(sender);
        if (!player)
            return;

        bool ok = false;
        string msg = GetTXGGarageService().AdminImpoundNearby(player, ok);
        Notify(player, msg, ok);
        SendGarageList(player);
    }

    // --- Server senders ---

    void SendGarageList(PlayerBase player)
    {
        if (!player || !player.GetIdentity())
            return;

        string uid = player.GetIdentity().GetPlainId();
        string name = player.GetIdentity().GetName();

        TXGSettings settings = GetTXGConfigService().GetServerSettings();
        int maxSlots = 5;
        if (settings)
            maxSlots = settings.maxVehicleStored;

        TXGGarageData data = TXGGarageData.Load(uid, name);

        TXGVehicleListFile payload = new TXGVehicleListFile();
        payload.ownerUID = uid;
        payload.maxSlots = maxSlots;
        payload.vehicles = GetTXGGarageService().BuildVehicleList(data);
        TXGVehicleRowVM nearbyRow = GetTXGGarageService().BuildNearbyRow(player);
        if (nearbyRow)
            payload.nearby.Insert(nearbyRow);
        GetTXGGarageService().BuildPeople(player, data, payload.people);
        payload.nowUnix = TXGInsuranceService.GetNowUnix();
        GetTXGGarageService().GetBalances(player, payload.bankBalance, payload.cashBalance);

        // Real object over the wire — DayZular marshals the nested ref array (no JSON string).
        GetDayZularRPCManager().SendRPC(TXGConstants.RPC_NAMESPACE, "OnGarageList", new Param1<ref TXGVehicleListFile>(payload), true, player.GetIdentity());
    }

    void Notify(PlayerBase player, string message, bool success)
    {
        if (!player || !player.GetIdentity() || message == "")
            return;
        TXGNotifyDTO dto = new TXGNotifyDTO(success, message);
        GetDayZularRPCManager().SendRPC(TXGConstants.RPC_NAMESPACE, "OnGarageNotify", new Param1<ref TXGNotifyDTO>(dto), true, player.GetIdentity());
    }

    // --- Client handlers ---

    void OnGarageList(PlayerIdentity sender, ParamsReadContext ctx)
    {
        Param1<ref TXGVehicleListFile> data = new Param1<ref TXGVehicleListFile>(null);
        if (!ctx.Read(data) || !data.param1)
            return;

        TXGVehicleListFile payload = data.param1;
        GetTXGGarageService().SetClientExtras(payload.people, payload.bankBalance, payload.cashBalance, payload.nowUnix);
        GetTXGGarageService().SetClientVehicles(payload.vehicles, payload.maxSlots, payload.nearby);
    }

    void OnGarageConfig(PlayerIdentity sender, ParamsReadContext ctx)
    {
        Param1<ref TXGConfigDTO> data = new Param1<ref TXGConfigDTO>(null);
        if (!ctx.Read(data) || !data.param1)
            return;

        TXGConfigDTO dto = data.param1;
        GetTXGConfigService().SetClientConfig(dto.parkInCost, dto.parkOutCost, dto.maxSlots, dto.npcClasses, dto.npcIds, dto.npcSpots, dto.enableRepairs, dto.enableInsurance, dto.insuranceDefaultDays, dto.isAdmin, dto.insuranceMaxDays);
    }

    void OnGarageNotify(PlayerIdentity sender, ParamsReadContext ctx)
    {
        Param1<ref TXGNotifyDTO> data = new Param1<ref TXGNotifyDTO>(null);
        if (!ctx.Read(data) || !data.param1)
            return;

        TXGNotifyDTO dto = data.param1;
        TXGGarageService.Event_OnNotify.Invoke(dto.message, dto.success);
    }

    // --- Helpers ---

    static PlayerBase GetPlayerByIdentity(PlayerIdentity identity)
    {
        if (!identity)
            return null;
        int low = 0;
        int high = 0;
        GetGame().GetPlayerNetworkIDByIdentityID(identity.GetPlayerId(), low, high);
        return PlayerBase.Cast(GetGame().GetObjectByNetworkId(low, high));
    }
}
