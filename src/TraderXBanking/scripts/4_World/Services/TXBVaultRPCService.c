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
			GetDayZularRPCManager().AddRPC(TXBConstants.RPC_NAMESPACE, "OnVaultToast", this, DayZularRPCType.Client);
		}
	}

	// --- Server: send vault data to requesting client ---

	void RequestVaultData(PlayerIdentity sender, ParamsReadContext ctx)
	{
		if (!sender) return;

		Param1<string> p = new Param1<string>("");
		if (!ctx.Read(p)) return;
		string ownerKey = p.param1;
		ownerKey = ResolveOwnerKey(sender, ownerKey);
		if (ownerKey == "") return;

		PlayerBase player = PlayerBase.Cast(GetPlayerByIdentity(sender));
		if (!player) return;

		GetTXBVaultService().SendVaultToClient(player, ownerKey);
	}

	// The client names the vault it wants; it may only open its own. Clan vaults have no membership
	// model yet, so any other key is refused (it would let a player read or empty someone else's vault).
	protected string ResolveOwnerKey(PlayerIdentity sender, string requested)
	{
		string own = "player:" + sender.GetPlainId();
		if (requested == "" || requested == own)
			return own;
		GetTXBLog().LogWarning("[Vault] " + sender.GetPlainId() + " requested vault '" + requested + "' that is not theirs: refused");
		return "";
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
		ownerKey = ResolveOwnerKey(sender, ownerKey);
		if (ownerKey == "") return;

		PlayerBase player = PlayerBase.Cast(GetPlayerByIdentity(sender));
		if (!player) return;

		bool ok = GetTXBVaultService().DepositItem(player, className, ownerKey);

		if (ok)
		{
			GetTXBVaultService().SendVaultToClient(player, ownerKey);
			GetTXBVaultService().SendVaultToast(player, "Vault", "Item sent to vault");
		}
	}

	// --- Server: withdraw specific item from vault ---

	void RequestVaultWithdraw(PlayerIdentity sender, ParamsReadContext ctx)
	{
		if (!sender) return;

		Param2<string, string> p = new Param2<string, string>("", "");
		if (!ctx.Read(p)) return;
		string itemId = p.param1;
		string ownerKey = p.param2;
		ownerKey = ResolveOwnerKey(sender, ownerKey);
		if (ownerKey == "") return;

		PlayerBase player = PlayerBase.Cast(GetPlayerByIdentity(sender));
		if (!player) return;

		bool ok = GetTXBVaultService().WithdrawItem(player, itemId, ownerKey);

		if (ok)
		{
			GetTXBVaultService().SendVaultToClient(player, ownerKey);
			GetTXBVaultService().SendVaultToast(player, "Vault", "Item retrieved from vault");
		}
	}

	// --- Server: withdraw all items ---

	void RequestVaultTakeAll(PlayerIdentity sender, ParamsReadContext ctx)
	{
		if (!sender) return;

		Param1<string> p = new Param1<string>("");
		if (!ctx.Read(p)) return;
		string ownerKey = p.param1;
		ownerKey = ResolveOwnerKey(sender, ownerKey);
		if (ownerKey == "") return;

		PlayerBase player = PlayerBase.Cast(GetPlayerByIdentity(sender));
		if (!player) return;

		int taken = GetTXBVaultService().TakeAll(player, ownerKey);
		GetTXBVaultService().SendVaultToClient(player, ownerKey);
		if (taken > 0)
			GetTXBVaultService().SendVaultToast(player, "Vault", taken.ToString() + " item(s) retrieved from vault");
	}

	// --- Client: receive vault data from server ---

	void OnVaultReceived(PlayerIdentity sender, ParamsReadContext ctx)
	{
		Param1<ref TXBVaultView> p = new Param1<ref TXBVaultView>(null);
		if (!ctx.Read(p)) return;

		TXBVaultView view = p.param1;
		if (!view) return;

		// Rebuild the local TXBVaultData from the lean DTO so the existing UI code is unchanged.
		TXBVaultData vault = new TXBVaultData();
		vault.ownerKey = view.ownerKey;
		vault.ownerType = "player";
		vault.capacity = view.capacity;
		vault.slotsUsed = view.slotsUsed;
		vault.feePerSlotPerDay = view.feePerSlotPerDay;

		if (view.items)
		{
			foreach (TXBVaultItemDTO dto : view.items)
			{
				if (!dto) continue;
				TXBVaultItem vi = new TXBVaultItem();
				vi.id = dto.id;
				vi.className = dto.className;
				vi.displayName = dto.displayName;
				vi.healthLevel = dto.condition;
				vi.quantity = dto.quantity;
				vi.footprintW = dto.footprintW;
				vi.footprintH = dto.footprintH;
				vi.depositorName = dto.depositorName;
				vi.depositedUtc = dto.depositedUtc;
				if (dto.hasAttachments) vi.attachmentsSummary = "Has attachments";
				vault.items.Insert(vi);
			}
		}

		if (view.activity)
		{
			foreach (TXBVaultActivityEntry ae : view.activity)
			{
				if (ae) vault.activity.Insert(ae);
			}
		}

		GetTXBLog().LogInfo("[TXB] OnVaultReceived OK | owner: " + vault.ownerKey + " | items: " + vault.items.Count().ToString() + " | slotsUsed: " + vault.slotsUsed.ToString());
		GetTXBVaultService().SetClientVault(vault);
	}

	// Client: transient toast confirming a deposit/withdraw.
	void OnVaultToast(PlayerIdentity sender, ParamsReadContext ctx)
	{
		Param2<string, string> p = new Param2<string, string>("", "");
		if (!ctx.Read(p))
			return;
		NotificationSystem.AddNotificationExtended(4, p.param1, p.param2, "");
	}
}
