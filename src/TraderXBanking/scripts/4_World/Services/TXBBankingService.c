// TraderXBanking - Banking Service
// Server-side transaction processing, money management, and account operations

class TXBBankingService extends DayZularService
{
	protected ref TXBTransactionQueue m_Queue = new TXBTransactionQueue();
	protected float m_TickAccumulator = 0;

	// Client-side account (synced from server)
	protected ref TXBAccountData m_ClientAccount;

	// Event fired when client account is updated (UI subscribes to this)
	static ref ScriptInvoker Event_OnAccountUpdated = new ScriptInvoker();

	void TXBBankingService()
	{
		GetDayZularEvents().OnUpdate.Insert(OnUpdate);

		if (GetGame().IsClient() || !GetGame().IsMultiplayer())
		{
			m_ClientAccount = new TXBAccountData();
		}
	}

	// --- Client-side account access ---

	TXBAccountData GetClientAccount()
	{
		return m_ClientAccount;
	}

	void SetClientAccount(TXBAccountData account)
	{
		m_ClientAccount = account;
		Event_OnAccountUpdated.Invoke();
	}

	// --- Server-side queue processing ---

	void OnUpdate(float timeslice)
	{
		if (!GetGame().IsServer())
			return;

		m_TickAccumulator += timeslice;
		if (m_TickAccumulator < TXB_QUEUE_INTERVAL)
			return;

		m_TickAccumulator = 0;

		if (!m_Queue.HasNext())
			return;

		TXBTransaction tx = m_Queue.Peek();
		if (tx)
		{
			ProcessTransaction(tx);
			m_Queue.Dequeue();
		}
	}

	void EnqueueTransaction(TXBTransaction transaction)
	{
		m_Queue.Enqueue(transaction);
	}

	// --- Transaction Processing ---

	protected void ProcessTransaction(TXBTransaction tx)
	{
		if (!tx.player || !tx.player.GetIdentity())
			return;

		TXBSettings settings = GetTXBConfigService().GetServerSettings();
		if (!settings)
			return;

		switch (tx.type)
		{
			case TXB_TRANSACTION_DEPOSIT:
				ProcessDeposit(tx, settings);
				break;
			case TXB_TRANSACTION_WITHDRAW:
				ProcessWithdraw(tx, settings);
				break;
			case TXB_TRANSACTION_TRANSFER:
				ProcessTransfer(tx, settings);
				break;
		}
	}

	protected void ProcessDeposit(TXBTransaction tx, TXBSettings settings)
	{
		string uid = tx.player.GetIdentity().GetPlainId();
		string name = tx.player.GetIdentity().GetName();
		ref TXBAccountData account = TXBAccountData.Load(uid, name, settings.defaultMaxBalance, settings.defaultStartBalance);

		int playerCash = GetPlayerMoney(tx.player, settings);
		if (playerCash < tx.amount)
		{
			DZLog().LogWarning("[TXB] Deposit failed: insufficient cash for " + name);
			return;
		}

		int afterFees = tx.amount - Math.Round(tx.amount * settings.transactionFeePercent);

		if (account.balance + afterFees > account.maxBalance)
		{
			DZLog().LogWarning("[TXB] Deposit failed: would exceed max balance for " + name);
			return;
		}

		if (!RemoveMoneyFromPlayer(tx.player, tx.amount, settings))
		{
			DZLog().LogWarning("[TXB] Deposit failed: could not remove money from " + name);
			return;
		}

		account.balance += afterFees;
		account.Save();
		SendAccountToClient(tx.player, account);

		if (settings.enableLogs)
			DZLog().LogInfo("[TXB] Deposit: " + name + " deposited " + tx.amount + " (after fees: " + afterFees + ") | balance: " + account.balance);
	}

	protected void ProcessWithdraw(TXBTransaction tx, TXBSettings settings)
	{
		string uid = tx.player.GetIdentity().GetPlainId();
		string name = tx.player.GetIdentity().GetName();
		ref TXBAccountData account = TXBAccountData.Load(uid, name, settings.defaultMaxBalance, settings.defaultStartBalance);

		if (account.balance < tx.amount)
		{
			DZLog().LogWarning("[TXB] Withdraw failed: insufficient balance for " + name);
			return;
		}

		account.balance -= tx.amount;

		if (!AddMoneyToPlayer(tx.player, tx.amount, settings))
		{
			account.balance += tx.amount;
			DZLog().LogWarning("[TXB] Withdraw failed: could not add money to " + name);
			return;
		}

		account.Save();
		SendAccountToClient(tx.player, account);

		if (settings.enableLogs)
			DZLog().LogInfo("[TXB] Withdraw: " + name + " withdrew " + tx.amount + " | balance: " + account.balance);
	}

	protected void ProcessTransfer(TXBTransaction tx, TXBSettings settings)
	{
		string uid = tx.player.GetIdentity().GetPlainId();
		string name = tx.player.GetIdentity().GetName();
		ref TXBAccountData srcAccount = TXBAccountData.Load(uid, name, settings.defaultMaxBalance, settings.defaultStartBalance);

		if (!TXBAccountData.AccountExists(tx.destinationUID))
		{
			DZLog().LogWarning("[TXB] Transfer failed: destination account not found " + tx.destinationUID);
			return;
		}

		int fees = Math.Round(tx.amount * settings.transactionFeePercent);
		int totalCost = tx.amount + fees;

		if (srcAccount.balance < totalCost)
		{
			DZLog().LogWarning("[TXB] Transfer failed: insufficient balance for " + name);
			return;
		}

		ref TXBAccountData destAccount = TXBAccountData.Load(tx.destinationUID, "", settings.defaultMaxBalance, settings.defaultStartBalance);

		int transferable = Math.Min(tx.amount, destAccount.maxBalance - destAccount.balance);
		if (transferable <= 0)
		{
			DZLog().LogWarning("[TXB] Transfer failed: destination account is full");
			return;
		}

		srcAccount.balance -= (transferable + fees);
		destAccount.balance += transferable;

		srcAccount.Save();
		destAccount.Save();
		SendAccountToClient(tx.player, srcAccount);

		if (settings.enableLogs)
			DZLog().LogInfo("[TXB] Transfer: " + name + " sent " + transferable + " to " + tx.destinationUID + " (fees: " + fees + ") | balance: " + srcAccount.balance);
	}

	// --- Money Helpers ---

	int GetPlayerMoney(PlayerBase player, TXBSettings settings)
	{
		int total = 0;
		array<EntityAI> items = new array<EntityAI>();
		player.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, items);

		foreach (EntityAI item : items)
		{
			string className = item.GetType();
			int value = settings.GetCurrencyValue(className);
			if (value > 0)
			{
				ItemBase itemBase = ItemBase.Cast(item);
				if (itemBase)
				{
					Magazine mag = Magazine.Cast(itemBase);
					if (mag)
						total += value * mag.GetAmmoCount();
					else
						total += value;
				}
			}
		}

		return total;
	}

	bool AddMoneyToPlayer(PlayerBase player, int amount, TXBSettings settings)
	{
		array<ref TXBCurrencyEntry> currencies = settings.GetCurrenciesSortedDesc();
		int remaining = amount;

		foreach (TXBCurrencyEntry entry : currencies)
		{
			if (remaining <= 0)
				break;

			int count = Math.Floor(remaining / entry.value);
			remaining -= count * entry.value;

			for (int i = 0; i < count; i++)
			{
				EntityAI spawned = player.GetInventory().CreateInInventory(entry.className);
				if (!spawned)
				{
					// Try to create on ground near player if inventory full
					spawned = EntityAI.Cast(GetGame().CreateObjectEx(entry.className, player.GetPosition(), ECE_PLACE_ON_SURFACE));
				}
			}
		}

		return true;
	}

	bool RemoveMoneyFromPlayer(PlayerBase player, int amount, TXBSettings settings)
	{
		int remaining = amount;
		array<ref TXBCurrencyEntry> currencies = settings.GetCurrenciesSortedAsc();

		array<EntityAI> items = new array<EntityAI>();
		player.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, items);

		// Collect removable currency items
		array<EntityAI> toRemove = new array<EntityAI>();
		array<int> toRemoveValues = new array<int>();

		foreach (EntityAI item : items)
		{
			if (remaining <= 0)
				break;

			int value = settings.GetCurrencyValue(item.GetType());
			if (value > 0)
			{
				toRemove.Insert(item);
				toRemoveValues.Insert(value);
				remaining -= value;
			}
		}

		if (remaining > 0)
			return false;

		// Remove items
		int totalRemoved = 0;
		for (int i = 0; i < toRemove.Count(); i++)
		{
			totalRemoved += toRemoveValues[i];
			GetGame().ObjectDelete(toRemove[i]);
		}

		// Give back overpayment
		int overpayment = totalRemoved - amount;
		if (overpayment > 0)
		{
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(AddMoneyToPlayerDelayed, 500, false, player, overpayment);
		}

		return true;
	}

	void AddMoneyToPlayerDelayed(PlayerBase player, int amount)
	{
		TXBSettings settings = GetTXBConfigService().GetServerSettings();
		if (settings && player)
			AddMoneyToPlayer(player, amount, settings);
	}

	// --- Account Sync ---

	void SendAccountToClient(PlayerBase player, TXBAccountData account)
	{
		if (!player || !player.GetIdentity())
			return;

		GetDayZularRPCManager().SendRPC(
			TXB_RPC_NAMESPACE,
			"OnAccountReceived",
			new Param3<int, int, string>(account.balance, account.maxBalance, account.playerUID),
			true,
			player.GetIdentity()
		);
	}

	// Get or create a player's account (server-side)
	ref TXBAccountData GetOrCreateAccount(PlayerBase player)
	{
		if (!player || !player.GetIdentity())
			return null;

		TXBSettings settings = GetTXBConfigService().GetServerSettings();
		string uid = player.GetIdentity().GetPlainId();
		string name = player.GetIdentity().GetName();

		return TXBAccountData.Load(uid, name, settings.defaultMaxBalance, settings.defaultStartBalance);
	}
}

// Global accessor
static ref TXBBankingService g_TXBBankingService;

TXBBankingService GetTXBBankingService()
{
	if (!g_TXBBankingService)
		g_TXBBankingService = TXBBankingService.Cast(DZlrServiceRegistry.GetInstance().GetService(TXBBankingService));
	return g_TXBBankingService;
}
