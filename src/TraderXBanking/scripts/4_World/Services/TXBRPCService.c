// TraderXBanking - RPC Service
// Handles all client-server RPC communication via DayZular RPC Manager

class TXBRPCService extends DayZularService
{
	void TXBRPCService()
	{
		GetDayZularEvents().OnPlayerJoined.Insert(OnPlayerJoined);
		RegisterRPCs();
	}

	protected void RegisterRPCs()
	{
		if (GetGame().IsServer() || !GetGame().IsMultiplayer())
		{
			// Server receives these from clients
			GetDayZularRPCManager().AddRPC(TXB_RPC_NAMESPACE, "RequestAccount", this, DayZularRPCType.Server);
			GetDayZularRPCManager().AddRPC(TXB_RPC_NAMESPACE, "RequestDeposit", this, DayZularRPCType.Server);
			GetDayZularRPCManager().AddRPC(TXB_RPC_NAMESPACE, "RequestWithdraw", this, DayZularRPCType.Server);
			GetDayZularRPCManager().AddRPC(TXB_RPC_NAMESPACE, "RequestTransfer", this, DayZularRPCType.Server);

			// Rate limit transactions: max 5 per second per player
			GetDayZularRPCManager().SetRateLimit(TXB_RPC_NAMESPACE, "RequestDeposit", 5, 1.0);
			GetDayZularRPCManager().SetRateLimit(TXB_RPC_NAMESPACE, "RequestWithdraw", 5, 1.0);
			GetDayZularRPCManager().SetRateLimit(TXB_RPC_NAMESPACE, "RequestTransfer", 5, 1.0);
		}

		if (GetGame().IsClient() || !GetGame().IsMultiplayer())
		{
			// Client receives these from server
			GetDayZularRPCManager().AddRPC(TXB_RPC_NAMESPACE, "OnAccountReceived", this, DayZularRPCType.Client);
			GetDayZularRPCManager().AddRPC(TXB_RPC_NAMESPACE, "OnSettingsReceived", this, DayZularRPCType.Client);
		}
	}

	// --- Server: Player join → send config ---

	void OnPlayerJoined(PlayerBase player, PlayerIdentity identity)
	{
		if (!GetGame().IsServer() || !identity)
			return;

		TXBSettings settings = GetTXBConfigService().GetServerSettings();
		if (!settings)
			return;

		// Send relevant config to the client (fee percent, credit card requirement)
		GetDayZularRPCManager().SendRPC(
			TXB_RPC_NAMESPACE,
			"OnSettingsReceived",
			new Param2<float, bool>(settings.transactionFeePercent, settings.requireCreditCard),
			true,
			identity
		);
	}

	// --- Server RPC Handlers ---

	void RequestAccount(PlayerIdentity sender, ParamsReadContext ctx)
	{
		if (!sender)
			return;

		PlayerBase player = PlayerBase.Cast(GetPlayerByUID(sender.GetPlainId()));
		if (!player)
			return;

		TXBBankingService bankService = GetTXBBankingService();
		ref TXBAccountData account = bankService.GetOrCreateAccount(player);
		if (account)
		{
			// Cap balance at max
			if (account.balance > account.maxBalance)
			{
				account.balance = account.maxBalance;
				account.Save();
			}
			bankService.SendAccountToClient(player, account);
		}
	}

	void RequestDeposit(PlayerIdentity sender, ParamsReadContext ctx)
	{
		Param1<int> data;
		if (!ctx.Read(data))
			return;

		PlayerBase player = PlayerBase.Cast(GetPlayerByUID(sender.GetPlainId()));
		if (!player)
			return;

		int amount = data.param1;
		if (amount <= 0)
			return;

		GetTXBBankingService().EnqueueTransaction(new TXBTransaction(player, amount, TXB_TRANSACTION_DEPOSIT));
	}

	void RequestWithdraw(PlayerIdentity sender, ParamsReadContext ctx)
	{
		Param1<int> data;
		if (!ctx.Read(data))
			return;

		PlayerBase player = PlayerBase.Cast(GetPlayerByUID(sender.GetPlainId()));
		if (!player)
			return;

		int amount = data.param1;
		if (amount <= 0)
			return;

		GetTXBBankingService().EnqueueTransaction(new TXBTransaction(player, amount, TXB_TRANSACTION_WITHDRAW));
	}

	void RequestTransfer(PlayerIdentity sender, ParamsReadContext ctx)
	{
		Param2<string, int> data;
		if (!ctx.Read(data))
			return;

		PlayerBase player = PlayerBase.Cast(GetPlayerByUID(sender.GetPlainId()));
		if (!player)
			return;

		string destUID = data.param1;
		int amount = data.param2;
		if (amount <= 0 || destUID == "")
			return;

		GetTXBBankingService().EnqueueTransaction(new TXBTransaction(player, amount, TXB_TRANSACTION_TRANSFER, destUID));
	}

	// --- Client RPC Handlers ---

	void OnAccountReceived(PlayerIdentity sender, ParamsReadContext ctx)
	{
		Param3<int, int, string> data;
		if (!ctx.Read(data))
			return;

		TXBAccountData account = GetTXBBankingService().GetClientAccount();
		if (account)
		{
			account.balance = data.param1;
			account.maxBalance = data.param2;
			account.playerUID = data.param3;
			GetTXBBankingService().SetClientAccount(account);
		}
	}

	void OnSettingsReceived(PlayerIdentity sender, ParamsReadContext ctx)
	{
		Param2<float, bool> data;
		if (!ctx.Read(data))
			return;

		TXBSettings clientSettings = GetTXBConfigService().GetClientSettings();
		if (clientSettings)
		{
			clientSettings.transactionFeePercent = data.param1;
			clientSettings.requireCreditCard = data.param2;
		}

		DZLog().LogInfo("[TXB] Client received server settings (fees: " + data.param1 + ")");
	}

	// --- Helpers ---

	protected Man GetPlayerByUID(string uid)
	{
		array<Man> players = new array<Man>();
		GetGame().GetPlayers(players);

		foreach (Man man : players)
		{
			if (man.GetIdentity() && man.GetIdentity().GetPlainId() == uid)
				return man;
		}

		return null;
	}
}
