// TraderXBanking - Home Tab Component (Refactored)
// Dashboard entry page using Factory Pattern with domain objects.
// All display data comes from TXB*Card Factory methods.

class TXBHomeTabComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("banking-home-tab", TXBHomeTabComponent);

	// NavButtonBinding calls SelectComponent on the host component.
	// This event bubbles the request up to TXBBankingComponent, which owns current_tab.
	static ref ScriptInvoker Event_OnNavigateRequest = new ScriptInvoker();

	// Domain Objects (Factory Pattern)
	// Hero section
	ref TXBHeroCard heroCard;
	
	// KPI Cards
	ref TXBKPIBalanceCard balanceCard;
	ref TXBKPIVaultCard vaultCard;
	ref TXBKPIStatsCard statsCard;

	// Activity & Alerts
	ref TXBRecentActivityViewModel activity;
	ref array<ref TXBAlert> alerts;
	string activityEmptyText;
	// Top-level array for the recent-activity ForEach (DayZular foreach can't resolve a nested
	// path like activity.visibleEntries — it binds a flat component member).
	ref array<ref TXBLedgerEntry> recentEntries;
	
	// Internal cache for calculations
	protected int m_TodayDelta;
	protected int m_Inflow30;
	protected int m_Outflow30;
	protected int m_TransferCount;
	protected int m_TotalTx;

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "banking-home-tab";
		m_Metadata.templatePath = "TraderXBanking/datasets/layouts/HomeTabView.layout";
	}

	override void OnInit()
	{
		// Initialize domain objects with empty states
		heroCard = new TXBHeroCard();
		balanceCard = new TXBKPIBalanceCard();
		vaultCard = new TXBKPIVaultCard();
		statsCard = TXBKPIStatsCard.Empty();
		activity = TXBRecentActivityViewModel.Empty();
		alerts = new array<ref TXBAlert>();
		recentEntries = new array<ref TXBLedgerEntry>();

		// Subscribe to service events
		TXBBankingService.Event_OnAccountUpdated.Insert(OnAccountUpdated);
		TXBBankingService.Event_OnLedgerUpdated.Insert(OnLedgerUpdated);
		TXBVaultService.Event_OnVaultUpdated.Insert(OnVaultUpdated);

		// Ask the server for the player's vault + ledger so the home shows real data
		// even if the Ledger tab hasn't been opened yet.
		RequestVaultData();
		RequestLedger();

		// Initial data load
		RefreshAll();
		MarkForCheck();
	}

	override void OnDestroy()
	{
		TXBBankingService.Event_OnAccountUpdated.Remove(OnAccountUpdated);
		TXBBankingService.Event_OnLedgerUpdated.Remove(OnLedgerUpdated);
		TXBVaultService.Event_OnVaultUpdated.Remove(OnVaultUpdated);
	}

	// Request the player's own vault (empty ownerKey > server resolves to player:<uid>).
	protected void RequestVaultData()
	{
		GetDayZularRPCManager().SendRPC(TXBConstants.RPC_NAMESPACE, "RequestVaultData", new Param1<string>(""), true, null);
	}

	protected void RequestLedger()
	{
		GetDayZularRPCManager().SendRPC(TXBConstants.RPC_NAMESPACE, "RequestLedger", new Param1<int>(100), true, null);
	}

	void OnVaultUpdated()
	{
		RefreshVaultCard();
		RefreshAlerts();
		MarkForCheck();
	}

	// Called by NavButtonBinding - bubble to the parent banking shell.
	void SelectComponent(string selector)
	{
		Event_OnNavigateRequest.Invoke(selector);
	}

	// --- Event handlers ---

	void OnAccountUpdated()
	{
		RefreshAll();
		MarkForCheck();
	}

	void OnLedgerUpdated()
	{
		RefreshAll();
		MarkForCheck();
	}

	// --- Main Refresh (Factory Pattern) ---

	protected void RefreshAll()
	{
		// 1. Hero Card
		RefreshHeroCard();
		
		// 2. Ledger stats (needed for balance delta and stats card)
		ComputeLedgerStats();
		
		// 3. KPI Cards via Factories
		RefreshBalanceCard();
		RefreshVaultCard();
		RefreshStatsCard();
		
		// 4. Activity & Alerts
		RefreshActivity();
		RefreshAlerts();
	}

	protected void RefreshHeroCard()
	{
		PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
		if (player)
		{
			PlayerIdentity identity = player.GetIdentity();
			if (identity)
			{
				heroCard = TXBHeroCard.FromIdentityAndSession(identity, 1);
				return;
			}
		}
		// Keep empty state if no player
		heroCard = new TXBHeroCard();
	}

	protected void RefreshBalanceCard()
	{
		TXBAccountData account = GetTXBBankingService().GetClientAccount();
		int cash = GetPlayerCash();
		balanceCard = TXBKPIBalanceCard.FromAccount(account, cash, m_TodayDelta);
	}

	protected void RefreshVaultCard()
	{
		// Real vault data synced from the server (null until OnVaultReceived fires > empty state).
		TXBVaultData vault = GetTXBVaultService().GetClientVault();
		vaultCard = TXBKPIVaultCard.FromVault(vault);
	}

	protected void RefreshStatsCard()
	{
		statsCard = TXBKPIStatsCard.FromStats(m_Inflow30, m_Outflow30, m_TransferCount, m_TotalTx);
	}

	protected void RefreshActivity()
	{
		array<ref TXBLedgerEntry> ledger = GetTXBBankingService().GetClientLedger();
		activity = TXBRecentActivityViewModel.FromLedger(ledger, 14);

		// Flatten into a top-level member for the ForEach binding.
		recentEntries.Clear();
		if (activity && activity.visibleEntries)
		{
			foreach (TXBLedgerEntry e : activity.visibleEntries)
			{
				if (e) recentEntries.Insert(e);
			}
		}

		if (recentEntries.Count() > 0)
			activityEmptyText = "";
		else
			activityEmptyText = "No recent activity";
	}

	protected void RefreshAlerts()
	{
		alerts.Clear();
		
		// Welcome alert for new players
		TXBAccountData account = GetTXBBankingService().GetClientAccount();
		if (account)
		{
			alerts.Insert(TXBAlert.Welcome(account.balance));
		}
		
		// Vault fee alert - only when the server actually charges storage fees.
		if (!vaultCard.isEmpty && vaultCard.feePerSlot > 0 && vaultCard.slotsUsed > 0 && vaultCard.IsFeeDueSoon())
		{
			int fee = vaultCard.feePerSlot * vaultCard.slotsUsed;
			alerts.Insert(TXBAlert.VaultFeeDue(fee, vaultCard.hoursUntilFee));
		}
	}

	// --- Helper Methods ---

	protected int GetPlayerCash()
	{
		PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
		if (!player) return 0;
		
		TXBSettings settings = GetTXBConfigService().GetClientSettings();
		if (!settings) return 0;
		
		return GetTXBBankingService().GetPlayerMoney(player, settings);
	}

	protected void ComputeLedgerStats()
	{
		array<ref TXBLedgerEntry> cache = GetTXBBankingService().GetClientLedger();
		
		if (!cache)
		{
			m_TodayDelta = 0;
			m_Inflow30 = 0;
			m_Outflow30 = 0;
			m_TransferCount = 0;
			m_TotalTx = 0;
			return;
		}

		int cutoff30 = TXBTimeUtil.DaysAgo(30);
		int cutoffToday = TXBTimeUtil.StartOfToday();

		m_TodayDelta = 0;
		m_Inflow30 = 0;
		m_Outflow30 = 0;
		m_TransferCount = 0;
		m_TotalTx = cache.Count();

		foreach (TXBLedgerEntry e : cache)
		{
			if (!e) continue;

			if (e.timestampUtc < cutoff30) continue;

			if (TXBLedgerEntryType.IsInflow(e.type))
				m_Inflow30 = m_Inflow30 + e.amount;
			else
				m_Outflow30 = m_Outflow30 + e.amount;

			if (e.type == TXBLedgerEntryType.TRANSFER_IN || e.type == TXBLedgerEntryType.TRANSFER_OUT)
				m_TransferCount = m_TransferCount + 1;

			if (e.timestampUtc >= cutoffToday)
			{
				if (TXBLedgerEntryType.IsInflow(e.type))
					m_TodayDelta = m_TodayDelta + e.amount;
				else
					m_TodayDelta = m_TodayDelta - e.amount;
			}
		}
	}

	// --- Event Handlers for Templates ---

	// Called when user clicks action button on an alert row
	// The _inputAlert variable is set by the template binding
	void OnAlertActionClicked()
	{
		if (!_inputAlert)
		{
			GetTXBLog().LogWarning("[TXB] Alert action clicked but _inputAlert is null");
			return;
		}

		GetTXBLog().LogInfo("[TXB] Alert action clicked: " + _inputAlert.title + " -> " + _inputAlert.actionTarget);

		// Route based on action target
		if (_inputAlert.actionTarget == "banking-account-tab")
		{
			// Navigate to account tab for deposit
			SelectComponent("banking-account-tab");
		}
		else if (_inputAlert.actionTarget == "banking-vault-tab")
		{
			// Navigate to vault tab
			SelectComponent("banking-vault-tab");
		}
		else if (_inputAlert.actionTarget == "dismiss")
		{
			// Dismiss this alert
			DismissAlert(_inputAlert.alertId);
		}
		else
		{
			GetTXBLog().LogWarning("[TXB] Unknown alert action target: " + _inputAlert.actionTarget);
		}
	}

	// Dismiss an alert by ID
	protected void DismissAlert(string alertId)
	{
		for (int i = 0; i < alerts.Count(); i++)
		{
			if (alerts[i] && alerts[i].alertId == alertId)
			{
				alerts.Remove(i);
				MarkForCheck();
				GetTXBLog().LogInfo("[TXB] Alert dismissed: " + alertId);
				return;
			}
		}
	}

	// Update alert title color based on alert type
	// Call this after alerts are rendered
	protected void UpdateAlertTitleColors()
	{
		Widget root = GetRoot();
		if (!root) return;

		for (int i = 0; i < alerts.Count(); i++)
		{
			TXBAlert alert = alerts[i];
			if (!alert) continue;

			// Find the title widget for this alert row
			// Naming convention: alertTitle_0, alertTitle_1, etc.
			string widgetName = "alertTitle_" + i.ToString();
			TextWidget titleWidget = TextWidget.Cast(root.FindAnyWidget(widgetName));
			
			if (titleWidget)
			{
				int colorHex = alert.GetTitleColorHex().ToInt();
				titleWidget.SetColor(colorHex);
			}
		}
	}

	// --- Template Input Variables ---
	// These are set by the DZML template bindings
	ref TXBAlert _inputAlert;
	ref TXBLedgerEntry _inputEntry;
}
