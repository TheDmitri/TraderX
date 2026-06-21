// TraderXBanking - Vault RPC Service
// Handles vault-specific client-server communication.

class TXBVaultRPCService extends DayZularService
{
	void TXBVaultRPCService()
	{
		RegisterRPCs();
	}

	protected void RegisterRPCs()
	{
		if (GetGame().IsServer() || !GetGame().IsMultiplayer())
		{
			GetDayZularRPCManager().AddRPC(TXBConstants.RPC_NAMESPACE, "RequestVaultData", this, DayZularRPCType.Server);
			GetDayZularRPCManager().AddRPC(TXBConstants.RPC_NAMESPACE, "RequestVaultDeposit", this, DayZularRPCType.Server);
			GetDayZularRPCManager().AddRPC(TXBConstants.RPC_NAMESPACE, "RequestVaultWithdraw", this, DayZularRPCType.Server);
			GetDayZularRPCManager().AddRPC(TXBConstants.RPC_NAMESPACE, "RequestVaultTakeAll", this, DayZularRPCType.Server);

			GetDayZularRPCManager().SetRateLimit(TXBConstants.RPC_NAMESPACE, "RequestVaultDeposit", 3, 1.0);
			GetDayZularRPCManager().SetRateLimit(TXBConstants.RPC_NAMESPACE, "RequestVaultWithdraw", 3, 1.0);
			GetDayZularRPCManager().SetRateLimit(TXBConstants.RPC_NAMESPACE, "RequestVaultData", 2, 1.0);
			GetDayZularRPCManager().SetRateLimit(TXBConstants.RPC_NAMESPACE, "RequestVaultTakeAll", 1, 2.0);
		}

		if (GetGame().IsClient() || !GetGame().IsMultiplayer())
		{
			GetDayZularRPCManager().AddRPC(TXBConstants.RPC_NAMESPACE, "OnVaultReceived", this, DayZularRPCType.Client);
		}
	}

	// --- Server: send vault data to requesting client ---

	void RequestVaultData(PlayerIdentity sender, ParamsReadContext ctx)
	{
		if (!sender) return;

		Param1<string> p = new Param1<string>("");
		if (!ctx.Read(p)) return;
		string ownerKey = p.param1;
		if (ownerKey == "") ownerKey = "player:" + sender.GetPlainId();

		PlayerBase player = PlayerBase.Cast(GetPlayerByIdentity(sender));
		if (!player) return;

		GetTXBVaultService().SendVaultToClient(player, ownerKey);
	}

	static PlayerBase GetPlayerByIdentity(PlayerIdentity identity)
    {
        int low = 0;
        int high = 0;
        GetGame().GetPlayerNetworkIDByIdentityID(identity.GetPlayerId(), low, high);
        return PlayerBase.Cast(GetGame().GetObjectByNetworkId(low, high));
    }

	// --- Server: deposit item from player inventory into vault ---

	void RequestVaultDeposit(PlayerIdentity sender, ParamsReadContext ctx)
	{
		if (!sender) return;

		Param2<string, string> p = new Param2<string, string>("", "");
		if (!ctx.Read(p)) return;
		string className = p.param1;
		string ownerKey = p.param2;
		if (ownerKey == "") ownerKey = "player:" + sender.GetPlainId();

		PlayerBase player = PlayerBase.Cast(GetPlayerByIdentity(sender));
		if (!player) return;

		bool ok = GetTXBVaultService().DepositItem(player, className, ownerKey);

		if (ok)
			GetTXBVaultService().SendVaultToClient(player, ownerKey);
	}

	// --- Server: withdraw specific item from vault ---

	void RequestVaultWithdraw(PlayerIdentity sender, ParamsReadContext ctx)
	{
		if (!sender) return;

		Param2<string, string> p = new Param2<string, string>("", "");
		if (!ctx.Read(p)) return;
		string itemId = p.param1;
		string ownerKey = p.param2;
		if (ownerKey == "") ownerKey = "player:" + sender.GetPlainId();

		PlayerBase player = PlayerBase.Cast(GetPlayerByIdentity(sender));
		if (!player) return;

		bool ok = GetTXBVaultService().WithdrawItem(player, itemId, ownerKey);

		if (ok)
			GetTXBVaultService().SendVaultToClient(player, ownerKey);
	}

	// --- Server: withdraw all items ---

	void RequestVaultTakeAll(PlayerIdentity sender, ParamsReadContext ctx)
	{
		if (!sender) return;

		Param1<string> p = new Param1<string>("");
		if (!ctx.Read(p)) return;
		string ownerKey = p.param1;
		if (ownerKey == "") ownerKey = "player:" + sender.GetPlainId();

		PlayerBase player = PlayerBase.Cast(GetPlayerByIdentity(sender));
		if (!player) return;

		GetTXBVaultService().TakeAll(player, ownerKey);
		GetTXBVaultService().SendVaultToClient(player, ownerKey);
	}

	// --- Client: receive vault data from server ---

	void OnVaultReceived(ParamsReadContext ctx)
	{
		Param1<string> p = new Param1<string>("");
		if (!ctx.Read(p)) return;
		string json = p.param1;
		if (json == "") return;

		TXBVaultData vault = new TXBVaultData();
		JsonSerializer js = new JsonSerializer();
		string err = "";
		js.ReadFromString(vault, json, err);

		if (err == "")
			GetTXBVaultService().SetClientVault(vault);
	}
}
