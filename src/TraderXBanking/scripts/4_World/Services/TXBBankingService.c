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

	// Event fired server-side after a ledger entry is persisted (for UI push / aggregates).
	// Also fired client-side after the client receives a synced ledger from the server.
	static ref ScriptInvoker Event_OnLedgerUpdated = new ScriptInvoker();
	static ref ScriptInvoker Event_OnContactsUpdated = new ScriptInvoker();

	// Client-side cache of recently synced ledger entries
	protected ref array<ref TXBLedgerEntry> m_ClientLedger;
	protected ref array<ref TXBContact> m_ClientContacts;

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

		// Flush ledger writes on interval (buffered persistence)
		TXBLedgerRepository.Tick(timeslice);

		m_TickAccumulator += timeslice;
		if (m_TickAccumulator < TXBConstants.QUEUE_INTERVAL)
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

	// --- Ledger helpers (server-side) ---

	// Record an entry to a player's ledger. Called internally after deposit/withdraw/transfer
	// and publicly via RecordExternalLedger() from the Trader and Garage modules.
	void RecordLedgerEntry(string uid, int entryType, int amount, int fee, int balanceAfter, string counterpartyName, string counterpartyUid, string location, string note)
	{
		if (!GetGame().IsServer()) return;
		if (uid == "") return;

		TXBLedgerEntry entry = TXBLedgerEntry.Create(entryType, amount, fee, balanceAfter, counterpartyName, counterpartyUid, location, note);

		TXBLedgerRepository.Append(uid, entry);
		Event_OnLedgerUpdated.Invoke(uid, entry);
	}

	// Cross-module entry point - called by TXBPaymentProvider (Trader) and GarageService.
	// Reads the player's current balance and records an appropriate ledger entry.
	void RecordExternalLedger(string uid, int entryType, int amount, int fee, string counterpartyName, string location, string note)
	{
		if (!GetGame().IsServer()) return;
		if (uid == "") return;

		TXBSettings settings = GetTXBConfigService().GetServerSettings();
		if (!settings) return;

		TXBAccountData account = TXBAccountData.Load(uid, "", settings.defaultMaxBalance, settings.defaultStartBalance);
		int balanceAfter = 0;
		if (account) balanceAfter = account.balance;

		RecordLedgerEntry(uid, entryType, amount, fee, balanceAfter, counterpartyName, "", location, note);
	}

	// --- Client-side ledger cache ---

	ref array<ref TXBLedgerEntry> GetClientLedger()
	{
		if (!m_ClientLedger)
			m_ClientLedger = new array<ref TXBLedgerEntry>();
		return m_ClientLedger;
	}

	void SetClientLedger(array<ref TXBLedgerEntry> entries)
	{
		m_ClientLedger = new array<ref TXBLedgerEntry>();
		if (entries)
		{
			foreach (TXBLedgerEntry e : entries)
			{
				if (e) m_ClientLedger.Insert(e);
			}
		}
		Event_OnLedgerUpdated.Invoke("", null);
	}

	ref array<ref TXBContact> GetClientContacts()
	{
		if (!m_ClientContacts)
			m_ClientContacts = new array<ref TXBContact>();
		return m_ClientContacts;
	}

	void SetClientContacts(array<ref TXBContact> contacts)
	{
		m_ClientContacts = new array<ref TXBContact>();
		if (contacts)
		{
			foreach (TXBContact c : contacts)
			{
				if (c) m_ClientContacts.Insert(c);
			}
		}
		Event_OnContactsUpdated.Invoke();
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
			case TXBConstants.TRANSACTION_DEPOSIT:
				ProcessDeposit(tx, settings);
				break;
			case TXBConstants.TRANSACTION_WITHDRAW:
				ProcessWithdraw(tx, settings);
				break;
			case TXBConstants.TRANSACTION_TRANSFER:
				ProcessTransfer(tx, settings);
				break;
		}
	}

	protected void ProcessDeposit(TXBTransaction tx, TXBSettings settings)
	{
		string uid = tx.player.GetIdentity().GetPlainId();
		string name = tx.player.GetIdentity().GetName();
		TXBAccountData account = TXBAccountData.Load(uid, name, settings.defaultMaxBalance, settings.defaultStartBalance);

		int playerCash = GetPlayerMoney(tx.player, settings);
		if (playerCash < tx.amount)
		{
			GetTXBLog().LogWarning("[TXB] Deposit failed: insufficient cash for " + name);
			return;
		}

		int afterFees = tx.amount - Math.Round(tx.amount * settings.transactionFeePercent);

		if (account.balance + afterFees > account.maxBalance)
		{
			GetTXBLog().LogWarning("[TXB] Deposit failed: would exceed max balance for " + name);
			return;
		}

		if (!RemoveMoneyFromPlayer(tx.player, tx.amount, settings))
		{
			GetTXBLog().LogWarning("[TXB] Deposit failed: could not remove money from " + name);
			return;
		}

		account.balance += afterFees;
		account.Save();
		SendAccountToClient(tx.player, account);

		int depositFee = tx.amount - afterFees;
		RecordLedgerEntry(uid, TXBLedgerEntryType.DEPOSIT, afterFees, depositFee, account.balance, name, uid, "", tx.note);

		if (settings.enableLogs)
			GetTXBLog().LogInfo("[TXB] Deposit: " + name + " deposited " + tx.amount + " (after fees: " + afterFees + ") | balance: " + account.balance);
	}

	protected void ProcessWithdraw(TXBTransaction tx, TXBSettings settings)
	{
		string uid = tx.player.GetIdentity().GetPlainId();
		string name = tx.player.GetIdentity().GetName();
		TXBAccountData account = TXBAccountData.Load(uid, name, settings.defaultMaxBalance, settings.defaultStartBalance);

		if (account.balance < tx.amount)
		{
			GetTXBLog().LogWarning("[TXB] Withdraw failed: insufficient balance for " + name);
			return;
		}

		account.balance -= tx.amount;

		if (!AddMoneyToPlayer(tx.player, tx.amount, settings))
		{
			account.balance += tx.amount;
			GetTXBLog().LogWarning("[TXB] Withdraw failed: could not add money to " + name);
			return;
		}

		account.Save();
		SendAccountToClient(tx.player, account);

		RecordLedgerEntry(uid, TXBLedgerEntryType.WITHDRAW, tx.amount, 0, account.balance, name, uid, "", tx.note);

		if (settings.enableLogs)
			GetTXBLog().LogInfo("[TXB] Withdraw: " + name + " withdrew " + tx.amount + " | balance: " + account.balance);
	}

	protected void ProcessTransfer(TXBTransaction tx, TXBSettings settings)
	{
		string uid = tx.player.GetIdentity().GetPlainId();
		string name = tx.player.GetIdentity().GetName();
		// Self-transfer would load the same file twice and the credit save would
		// overwrite the debit save, minting the amount.
		if (tx.amount <= 0 || tx.destinationUID == uid)
		{
			GetTXBLog().LogWarning("[TXB] Transfer rejected: invalid amount or self-transfer from " + name);
			return;
		}

		TXBAccountData srcAccount = TXBAccountData.Load(uid, name, settings.defaultMaxBalance, settings.defaultStartBalance);

		// Bounding amount by balance keeps amount + fees far from int overflow.
		if (tx.amount > srcAccount.balance)
		{
			GetTXBLog().LogWarning("[TXB] Transfer failed: insufficient balance for " + name);
			return;
		}

		if (!TXBAccountData.AccountExists(tx.destinationUID))
		{
			GetTXBLog().LogWarning("[TXB] Transfer failed: destination account not found " + tx.destinationUID);
			return;
		}

		int fees = Math.Round(tx.amount * settings.transactionFeePercent);
		int totalCost = tx.amount + fees;

		if (srcAccount.balance < totalCost)
		{
			GetTXBLog().LogWarning("[TXB] Transfer failed: insufficient balance for " + name);
			return;
		}

		TXBAccountData destAccount = TXBAccountData.Load(tx.destinationUID, "", settings.defaultMaxBalance, settings.defaultStartBalance);

		int transferable = Math.Min(tx.amount, destAccount.maxBalance - destAccount.balance);
		if (transferable <= 0)
		{
			GetTXBLog().LogWarning("[TXB] Transfer failed: destination account is full");
			return;
		}

		srcAccount.balance -= (transferable + fees);
		destAccount.balance += transferable;

		srcAccount.Save();
		destAccount.Save();
		SendAccountToClient(tx.player, srcAccount);

		// Ledger: two entries - TRANSFER_OUT on sender, TRANSFER_IN on recipient.
		string destName = destAccount.playerName;
		RecordLedgerEntry(uid, TXBLedgerEntryType.TRANSFER_OUT, transferable, fees, srcAccount.balance, destName, tx.destinationUID, "", tx.note);
		RecordLedgerEntry(tx.destinationUID, TXBLedgerEntryType.TRANSFER_IN, transferable, 0, destAccount.balance, name, uid, "", tx.note);

		// Contacts: auto-populate sender's contact book with the recipient.
		TXBContactData senderContacts = TXBContactData.Load(uid);
		senderContacts.RecordTransfer(tx.destinationUID, destName, TXBLedgerEntry.GetUtcStamp());
		senderContacts.Save();
		SendContactsToClient(tx.player);

		if (settings.enableLogs)
			GetTXBLog().LogInfo("[TXB] Transfer: " + name + " sent " + transferable + " to " + tx.destinationUID + " (fees: " + fees + ") | balance: " + srcAccount.balance);
	}

	// --- Money Helpers ---

	int GetPlayerMoney(Man player, TXBSettings settings)
	{
		int total = 0;
		array<EntityAI> items = new array<EntityAI>();
		player.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, items);

		foreach (EntityAI item : items)
		{
			int value = settings.GetCurrencyValue(item.GetType());
			if (value <= 0)
				continue;

			ItemBase itemBase = ItemBase.Cast(item);
			if (itemBase)
				total += value * CurrencyQty(itemBase);   // value * stack size (bills) / ammo count (coins)
		}

		return total;
	}

	// Universal stack size for a currency item: ammo count for coins (Magazine), quantity for
	// stackable bills, 1 otherwise. Mirrors TraderX's TraderXQuantityManager.GetItemAmount.
	protected int CurrencyQty(ItemBase item)
	{
		int amount = 1;
		if (item.HasQuantity())
		{
			Magazine mag;
			if (Class.CastTo(mag, item))
				amount = mag.GetAmmoCount();
			else
				amount = item.GetQuantity();
		}
		if (amount < 1)
			amount = 1;
		return amount;
	}

	protected void SetCurrencyQty(ItemBase item, int amount)
	{
		Magazine mag;
		if (Class.CastTo(mag, item))
			mag.ServerSetAmmoCount(amount);
		else
			item.SetQuantity(amount);
	}

	bool AddMoneyToPlayer(Man player, int amount, TXBSettings settings)
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

	bool RemoveMoneyFromPlayer(Man player, int amount, TXBSettings settings)
	{
		array<EntityAI> items = new array<EntityAI>();
		player.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, items);

		// Pre-check: ensure the player actually has enough (value * stack size across all currency).
		int available = 0;
		foreach (EntityAI it : items)
		{
			int v0 = settings.GetCurrencyValue(it.GetType());
			if (v0 <= 0) continue;
			ItemBase ib0 = ItemBase.Cast(it);
			if (ib0) available += v0 * CurrencyQty(ib0);
		}
		if (available < amount)
			return false;

		// Deduct smallest denominations first to minimize change. For each currency item, take only
		// the units needed from its stack (reduce quantity) instead of deleting the whole stack.
		int remaining = amount;
		int totalRemoved = 0;
		array<ref TXBCurrencyEntry> currencies = settings.GetCurrenciesSortedAsc();

		foreach (TXBCurrencyEntry cur : currencies)
		{
			if (remaining <= 0) break;

			foreach (EntityAI item : items)
			{
				if (remaining <= 0) break;
				if (item.GetType() != cur.className) continue;

				ItemBase ib = ItemBase.Cast(item);
				if (!ib) continue;

				int qty = CurrencyQty(ib);
				int unitsNeeded = Math.Ceil(remaining / (cur.value * 1.0));
				int take = Math.Min(unitsNeeded, qty);

				totalRemoved += take * cur.value;
				remaining -= take * cur.value;

				if (take >= qty)
					GetGame().ObjectDelete(item);
				else
					SetCurrencyQty(ib, qty - take);
			}
		}

		if (remaining > 0)
			return false;

		// Give back overpayment (e.g. broke a 5000 bill for a 100 withdrawal).
		int overpayment = totalRemoved - amount;
		if (overpayment > 0)
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(AddMoneyToPlayerDelayed, 500, false, player, overpayment);

		return true;
	}

	void AddMoneyToPlayerDelayed(Man player, int amount)
	{
		TXBSettings settings = GetTXBConfigService().GetServerSettings();
		if (settings && player)
			AddMoneyToPlayer(player, amount, settings);
	}

	// --- Account Sync ---

	void SendAccountToClient(Man player, TXBAccountData account)
	{
		if (!player || !player.GetIdentity())
			return;

		GetDayZularRPCManager().SendRPC( TXBConstants.RPC_NAMESPACE, "OnAccountReceived", new Param3<int, int, string>(account.balance, account.maxBalance, account.playerUID), true, player.GetIdentity() );
	}

	void SendContactsToClient(Man player)
	{
		if (!player || !player.GetIdentity())
			return;
		string uid = player.GetIdentity().GetPlainId();
		TXBContactData cdata = TXBContactData.Load(uid);
		if (!cdata || !cdata.contacts)
			return;
		GetDayZularRPCManager().SendRPC( TXBConstants.RPC_NAMESPACE, "OnContactsReceived", new Param1<ref array<ref TXBContact>>(cdata.contacts), true, player.GetIdentity() );
	}

	// Send the last N ledger entries to a client. Entries are JSON-serialised into a
	// single string payload - simple and within DayZ RPC size budget for ~50 entries.
	void SendLedgerToClient(Man player, int maxEntries)
	{
		if (!player || !player.GetIdentity())
			return;

		string uid = player.GetIdentity().GetPlainId();
		TXBLedgerFile file = TXBLedgerRepository.Load(uid);
		if (!file || !file.entries)
			return;

		TXBLedgerFile payload = new TXBLedgerFile();
		payload.playerUID = uid;
		int count = file.entries.Count();
		int start = 0;
		if (count > maxEntries) start = count - maxEntries;
		int i = start;
		while (i < count)
		{
			payload.entries.Insert(file.entries.Get(i));
			i = i + 1;
		}

		// Send the entries as an array of ref objects (DayZular marshals each element over RPC).
		// TXBLedgerEntry is a flat primitive/string model - no JSON string, no RPC size limit.
		GetDayZularRPCManager().SendRPC( TXBConstants.RPC_NAMESPACE, "OnLedgerReceived", new Param1<ref array<ref TXBLedgerEntry>>(payload.entries), true, player.GetIdentity() );
	}

	// Get or create a player's account (server-side)
	ref TXBAccountData GetOrCreateAccount(Man player)
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
