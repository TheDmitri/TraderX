// TraderXBanking - Account Tab Component
// Backs the Account tab (AccountTabView.layout).
// Handles deposit, withdraw, copy-id and rolls up 30-day stats from the ledger.

class TXBAccountTabComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("banking-account-tab", TXBAccountTabComponent);

	// Balance card
	string bankBalanceText;
	string maxBalanceText;
	string playerIdText;

	// Forms (two-way)
	string depositAmount;
	string withdrawAmount;

	// Deposit section
	string feesText;
	string depositStatusText;

	// Withdraw section
	string statusText;

	// Stats (computed from ledger)
	string statDepositsText;
	string statWithdrawalsText;
	string statTransfersText;

	// Recent transactions (last 10 entries)
	string recentEmptyText;
	ref array<ref TXBLedgerEntry> _recentTransactions;

	// Internal state
	protected int m_BankBalance;
	protected int m_MaxBalance;
	protected int m_PlayerCash;
	protected string m_PlayerUID;
	protected float m_FeePercent;
	protected float m_ThrottleTimer;
	protected const float THROTTLE_INTERVAL = 1.0;

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "banking-account-tab";
		m_Metadata.templatePath = "TraderXBanking/datasets/layouts/AccountTabView.layout";
	}

	override void OnInit()
	{
		_recentTransactions = new array<ref TXBLedgerEntry>();

		PlayerIdentity identity = GetGame().GetPlayer().GetIdentity();
		if (identity)
			m_PlayerUID = identity.GetPlainId();

		TXBSettings clientSettings = GetTXBConfigService().GetClientSettings();
		if (clientSettings)
			m_FeePercent = clientSettings.transactionFeePercent;

		feesText = "On hand: — ₽ · Fee: " + Math.Round(m_FeePercent * 100).ToString() + "%";
		depositStatusText = "";
		statusText = "You must carry a valid credit card to withdraw.";
		recentEmptyText = "No transactions yet";
		playerIdText = m_PlayerUID;
		depositAmount = "";
		withdrawAmount = "";

		TXBBankingService.Event_OnAccountUpdated.Insert(OnAccountUpdated);
		TXBBankingService.Event_OnLedgerUpdated.Insert(OnLedgerUpdated);

		GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(PeriodicUpdate, 500, true);

		RefreshFromAccount();
		RecomputeStats();
		MarkForCheck();
	}

	override void OnDestroy()
	{
		TXBBankingService.Event_OnAccountUpdated.Remove(OnAccountUpdated);
		TXBBankingService.Event_OnLedgerUpdated.Remove(OnLedgerUpdated);
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(PeriodicUpdate);
	}

	// --- Account sync ---

	void OnAccountUpdated()
	{
		RefreshFromAccount();
		MarkForCheck();
	}

	void OnLedgerUpdated()
	{
		RecomputeStats();
		MarkForCheck();
	}

	protected void RefreshFromAccount()
	{
		TXBAccountData account = GetTXBBankingService().GetClientAccount();
		if (account)
		{
			m_BankBalance = account.balance;
			m_MaxBalance = account.maxBalance;
		}
		UpdateBalanceTexts();
		UpdateProgressBar();
	}

	protected void UpdateBalanceTexts()
	{
		bankBalanceText = m_BankBalance.ToString();
		maxBalanceText = "MAX " + m_MaxBalance.ToString();
	}

	protected void UpdateProgressBar()
	{
		Widget fill = null;
		Widget root = GetRoot();
		if (root) fill = root.FindAnyWidget("progressFill");
		if (!fill) return;

		float ratio = 0;
		if (m_MaxBalance > 0)
			ratio = Math.Clamp(m_BankBalance / (m_MaxBalance * 1.0), 0, 1);

		fill.SetSize(ratio, 1);
	}

	// --- Periodic ---

	void PeriodicUpdate()
	{
		UpdatePlayerCash();

		if (m_ThrottleTimer > 0)
			m_ThrottleTimer -= 0.5;

		MarkForCheck();
	}

	protected void UpdatePlayerCash()
	{
		PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
		if (!player) return;

		TXBSettings settings = GetTXBConfigService().GetClientSettings();
		if (!settings) return;

		m_PlayerCash = GetTXBBankingService().GetPlayerMoney(player, settings);
		feesText = "On hand: " + m_PlayerCash.ToString() + " ₽ · Fee: " + Math.Round(m_FeePercent * 100).ToString() + "%";
	}

	protected bool CanTransact()
	{
		if (m_ThrottleTimer > 0)
		{
			statusText = "Please wait...";
			MarkForCheck();
			return false;
		}
		return true;
	}

	protected void StartThrottle()
	{
		m_ThrottleTimer = THROTTLE_INTERVAL;
	}

	// --- Deposit ---

	void OnDeposit()
	{
		if (!CanTransact()) return;

		int amount = depositAmount.ToInt();
		if (amount <= 0)
		{
			depositStatusText = "Enter a valid amount.";
			MarkForCheck();
			return;
		}

		if (amount > m_PlayerCash)
		{
			depositStatusText = "Not enough cash — you have " + m_PlayerCash.ToString() + " ₽.";
			MarkForCheck();
			return;
		}

		int afterFees = amount - Math.Round(amount * m_FeePercent);
		if (m_BankBalance + afterFees > m_MaxBalance)
		{
			depositStatusText = "Would exceed max balance of " + m_MaxBalance.ToString() + " ₽.";
			MarkForCheck();
			return;
		}

		GetDayZularRPCManager().SendRPC(TXBConstants.RPC_NAMESPACE, "RequestDeposit", new Param1<int>(amount), true, null);

		depositStatusText = "";
		depositAmount = "";
		StartThrottle();
		MarkForCheck();
	}

	void OnDepositAmountChanged(string text)
	{
		depositStatusText = "";
		int amount = text.ToInt();
		if (amount > 0)
		{
			int afterFees = amount - Math.Round(amount * m_FeePercent);
			feesText = "On hand: " + m_PlayerCash.ToString() + " ₽ · Receive: " + afterFees.ToString() + " (fee " + (amount - afterFees).ToString() + ")";
		}
		else
		{
			feesText = "On hand: " + m_PlayerCash.ToString() + " ₽ · Fee: " + Math.Round(m_FeePercent * 100).ToString() + "%";
		}
		MarkForCheck();
	}

	// --- Withdraw ---

	void OnWithdraw()
	{
		if (!CanTransact()) return;

		int amount = withdrawAmount.ToInt();
		if (amount <= 0)
		{
			statusText = "Enter a valid amount.";
			MarkForCheck();
			return;
		}

		if (amount > m_BankBalance)
		{
			statusText = "Insufficient bank balance.";
			MarkForCheck();
			return;
		}

		GetDayZularRPCManager().SendRPC(TXBConstants.RPC_NAMESPACE, "RequestWithdraw", new Param1<int>(amount), true, null);

		statusText = "Withdrawing " + amount.ToString() + "...";
		withdrawAmount = "";
		StartThrottle();
		MarkForCheck();
	}

	void OnWithdrawAmountChanged(string text)
	{
		int amount = text.ToInt();
		if (amount <= 0)
			statusText = "You must carry a valid credit card to withdraw.";
		else if (amount > m_BankBalance)
			statusText = "Insufficient balance (have " + m_BankBalance.ToString() + " ₽).";
		else
			statusText = "Withdraw " + amount.ToString() + " ₽ to your inventory.";
		MarkForCheck();
	}

	// --- Copy ID ---

	void OnCopyId()
	{
		GetGame().CopyToClipboard(m_PlayerUID);
		statusText = "Account ID copied to clipboard.";
		MarkForCheck();
	}

	// --- Stats + Recent transactions (30-day rolling from the client ledger cache) ---

	protected void RecomputeStats()
	{
		int deposits = 0;
		int withdrawals = 0;
		int transfers = 0;

		_recentTransactions.Clear();

		array<ref TXBLedgerEntry> cache = GetTXBBankingService().GetClientLedger();
		int cutoff = ComputeCutoffStamp30Days();

		if (cache)
		{
			foreach (TXBLedgerEntry e : cache)
			{
				if (!e) continue;
				if (e.timestampUtc < cutoff) continue;

				if (e.type == TXBLedgerEntryType.DEPOSIT) deposits = deposits + e.amount;
				else if (e.type == TXBLedgerEntryType.WITHDRAW) withdrawals = withdrawals + e.amount;
				else if (e.type == TXBLedgerEntryType.TRANSFER_OUT || e.type == TXBLedgerEntryType.TRANSFER_IN) transfers = transfers + 1;
			}

			// Populate recent transactions (last 10, most recent first)
			int startIdx = cache.Count() - 10;
			if (startIdx < 0) startIdx = 0;
			int ri = cache.Count() - 1;
			while (ri >= startIdx)
			{
				TXBLedgerEntry entry = cache[ri];
				if (entry) _recentTransactions.Insert(entry);
				ri = ri - 1;
			}
		}

		statDepositsText = "+" + deposits.ToString();
		statWithdrawalsText = "-" + withdrawals.ToString();
		statTransfersText = transfers.ToString();
		recentEmptyText = "";
		if (_recentTransactions.Count() == 0)
			recentEmptyText = "No transactions yet";
	}

	// Cutoff = now - 30 days, expressed as the same YYYYMMDDhhmmss int used by TXBLedgerEntry.
	protected int ComputeCutoffStamp30Days()
	{
		int y;
		int m;
		int d;
		int h;
		int mn;
		int s;
		GetYearMonthDayUTC(y, m, d);
		GetHourMinuteSecondUTC(h, mn, s);

		// Naive 30-day subtraction on day counter; borrow months when needed.
		d = d - 30;
		while (d < 1)
		{
			m = m - 1;
			if (m < 1)
			{
				m = 12;
				y = y - 1;
			}
			d = d + DaysInMonth(y, m);
		}
		return y * 10000000000 + m * 100000000 + d * 1000000 + h * 10000 + mn * 100 + s;
	}

	protected int DaysInMonth(int year, int month)
	{
		if (month == 2)
		{
			bool leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
			if (leap) return 29;
			return 28;
		}
		if (month == 4 || month == 6 || month == 9 || month == 11) return 30;
		return 31;
	}
}
