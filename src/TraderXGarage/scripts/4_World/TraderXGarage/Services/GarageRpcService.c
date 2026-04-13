// TraderXGarage - GarageRpcService
// Handles RPC registration and routing for garage operations.
// Uses DayZular RPC manager with rate limiting.

class GarageRpcService extends DayZularService
{
	private ref GarageSettings m_ClientSettings;

	void GarageRpcService()
	{
		GetDayZularEvents().OnMissionStart.Insert(OnMissionStart);
	}

	void OnMissionStart()
	{
		RegisterRpcEndpoints();
	}

	ref GarageSettings GetClientSettings()
	{
		return m_ClientSettings;
	}

	// ─── RPC Registration ───

	private void RegisterRpcEndpoints()
	{
		if (GetGame().IsServer())
		{
			GetDayZularRPCManager().AddRPC(TXG_RPC_NAMESPACE, TXG_RPC_GARAGE_REQUEST, this, SingleplayerExecutionType.Both);
			GetDayZularRPCManager().AddRPC(TXG_RPC_NAMESPACE, TXG_RPC_PARK_IN, this, SingleplayerExecutionType.Both);
			GetDayZularRPCManager().AddRPC(TXG_RPC_NAMESPACE, TXG_RPC_PARK_OUT, this, SingleplayerExecutionType.Both);

			GetDayZularRPCManager().SetRateLimit(TXG_RPC_NAMESPACE, TXG_RPC_GARAGE_REQUEST, TXG_RPC_RATE_LIMIT, TXG_RPC_RATE_WINDOW);
			GetDayZularRPCManager().SetRateLimit(TXG_RPC_NAMESPACE, TXG_RPC_PARK_IN, TXG_RPC_RATE_LIMIT, TXG_RPC_RATE_WINDOW);
			GetDayZularRPCManager().SetRateLimit(TXG_RPC_NAMESPACE, TXG_RPC_PARK_OUT, TXG_RPC_RATE_LIMIT, TXG_RPC_RATE_WINDOW);
		}

		if (GetGame().IsClient() || !GetGame().IsMultiplayer())
		{
			GetDayZularRPCManager().AddRPC(TXG_RPC_NAMESPACE, TXG_RPC_GARAGE_RESPONSE, this, SingleplayerExecutionType.Both);
			GetDayZularRPCManager().AddRPC(TXG_RPC_NAMESPACE, TXG_RPC_SYNC_CONFIG, this, SingleplayerExecutionType.Both);
			GetDayZularRPCManager().AddRPC(TXG_RPC_NAMESPACE, "Notification", this, SingleplayerExecutionType.Both);
		}
	}

	// ─── Server-side RPC Handlers ───

	void OnRPC_GarageRequest(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (!GetGame().IsServer())
			return;

		Param3<int, vector, bool> data;
		if (!ctx.Read(data))
			return;

		PlayerBase player = GetPlayerByIdentity(sender);
		if (!player)
			return;

		GarageService garageService = GetGarageService();
		if (garageService)
			garageService.HandleGarageRequest(player, data.param1, data.param2, data.param3);
	}

	void OnRPC_ParkIn(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (!GetGame().IsServer())
			return;

		Param3<int, CarScript, vector> data;
		if (!ctx.Read(data))
			return;

		PlayerBase player = GetPlayerByIdentity(sender);
		if (!player)
			return;

		GarageService garageService = GetGarageService();
		if (garageService)
			garageService.HandleParkIn(player, data.param1, data.param2, data.param3);
	}

	void OnRPC_ParkOut(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (!GetGame().IsServer())
			return;

		Param4<int, int, string, vector> data;
		if (!ctx.Read(data))
			return;

		PlayerBase player = GetPlayerByIdentity(sender);
		if (!player)
			return;

		GarageService garageService = GetGarageService();
		if (garageService)
			garageService.HandleParkOut(player, data.param1, data.param2, data.param3, data.param4);
	}

	// ─── Client-side RPC Handlers ───

	void OnRPC_GarageResponse(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (!GetGame().IsClient() && GetGame().IsMultiplayer())
			return;

		Param4<ref TStringArray, ref TIntArray, vector, int> data;
		if (!ctx.Read(data))
			return;

		GarageUIService uiService = GetGarageUIService();
		if (uiService)
			uiService.OnGarageResponse(data.param1, data.param2, data.param3, data.param4);
	}

	void OnRPC_SyncConfig(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (!GetGame().IsClient() && GetGame().IsMultiplayer())
			return;

		Param1<ref GarageSettings> data;
		if (!ctx.Read(data))
			return;

		m_ClientSettings = data.param1;
	}

	void OnRPC_Notification(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (!GetGame().IsClient() && GetGame().IsMultiplayer())
			return;

		Param1<string> data;
		if (!ctx.Read(data))
			return;

		// Display notification to the player via DayZ notification system
		NotificationSystem.AddNotification(NotificationType.GENERIC, NotificationSystem.DEFAULT_TIME_DISPLAYED, "Garage", data.param1);
	}

	// ─── Utility ───

	private PlayerBase GetPlayerByIdentity(PlayerIdentity identity)
	{
		if (!identity)
			return null;

		int lowBits, highBits;
		GetGame().GetPlayerNetworkID(identity, lowBits, highBits);
		return PlayerBase.Cast(GetGame().GetObjectByNetworkId(lowBits, highBits));
	}
}

// ─── Global Accessors ───

static GarageService GetGarageService()
{
	return GarageService.Cast(DZlrServiceRegistry.GetInstance().GetService(GarageService));
}

static GarageRpcService GetGarageRpcService()
{
	return GarageRpcService.Cast(DZlrServiceRegistry.GetInstance().GetService(GarageRpcService));
}

static GarageUIService GetGarageUIService()
{
	return GarageUIService.Cast(DZlrServiceRegistry.GetInstance().GetService(GarageUIService));
}
