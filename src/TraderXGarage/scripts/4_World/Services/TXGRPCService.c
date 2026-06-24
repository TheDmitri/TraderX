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

            GetDayZularRPCManager().SetRateLimit(TXGConstants.RPC_NAMESPACE, "RequestGarage", 4, 1.0);
            GetDayZularRPCManager().SetRateLimit(TXGConstants.RPC_NAMESPACE, "RequestParkIn", 3, 1.0);
            GetDayZularRPCManager().SetRateLimit(TXGConstants.RPC_NAMESPACE, "RequestParkOut", 3, 1.0);
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
        foreach (TXGNpcEntry npc : settings.npcs)
        {
            if (npc && npc.className != "")
                dto.npcClasses.Insert(npc.className);
        }

        GetDayZularRPCManager().SendRPC(TXGConstants.RPC_NAMESPACE, "OnGarageConfig", new Param1<ref TXGConfigDTO>(dto), true, identity);
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
        GetTXGGarageService().SetClientVehicles(payload.vehicles, payload.maxSlots);
    }

    void OnGarageConfig(PlayerIdentity sender, ParamsReadContext ctx)
    {
        Param1<ref TXGConfigDTO> data = new Param1<ref TXGConfigDTO>(null);
        if (!ctx.Read(data) || !data.param1)
            return;

        TXGConfigDTO dto = data.param1;
        GetTXGConfigService().SetClientConfig(dto.parkInCost, dto.parkOutCost, dto.maxSlots, dto.npcClasses);
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
