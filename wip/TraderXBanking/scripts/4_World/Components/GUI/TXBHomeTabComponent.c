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
	ref TXBKPIClanCard clanCard;
	ref TXBKPIStatsCard statsCard;
	
	// Activity & Alerts
	ref TXBRecentActivityViewModel activity;
	ref array<ref TXBAlert> alerts;
	ref array<ref TXBClanInvite> pendingInvites;
	
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
		clanCard = TXBKPIClanCard.Empty();
		statsCard = TXBKPIStatsCard.Empty();
		activity = TXBRecentActivityViewModel.Empty();
		alerts = new array<ref TXBAlert>();
		pendingInvites = new array<ref TXBClanInvite>();

		// Subscribe to service events
		TXBBankingService.Event_OnAccountUpdated.Insert(OnAccountUpdated);
		TXBBankingService.Event_OnLedgerUpdated.Insert(OnLedgerUpdated);

		// Initial data load
		RefreshAll();
		MarkForCheck();
	}

	override void OnDestroy()
	{
		TXBBankingService.Event_OnAccountUpdated.Remove(OnAccountUpdated);
		TXBBankingService.Event_OnLedgerUpdated.Remove(OnLedgerUpdated);
	}

	// Called by NavButtonBinding — bubble to the parent banking shell.
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
		RefreshClanCard();
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
		// For now, use empty state (TXBVaultService not fully implemented)
		vaultCard = new TXBKPIVaultCard();
		vaultCard.isEmpty = true;
		vaultCard.emptyMessage = "No vault configured";
		vaultCard.valueText = "— / —";
		vaultCard.subText = "Rent a vault box from a banker";
		vaultCard.footerLeft = "—";
	}

	protected void RefreshClanCard()
	{
		// For now, use empty state (TXBClanService not implemented)
		clanCard = TXBKPIClanCard.Empty();
	}

	protected void RefreshStatsCard()
	{
		statsCard = TXBKPIStatsCard.FromStats(m_Inflow30, m_Outflow30, m_TransferCount, m_TotalTx);
	}

	protected void RefreshActivity()
	{
		array<ref TXBLedgerEntry> ledger = GetTXBBankingService().GetClientLedger();
		activity = TXBRecentActivityViewModel.FromLedger(ledger, 6);
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
		
		// Vault fee alert if applicable
		if (!vaultCard.isEmpty && vaultCard.IsFeeDueSoon())
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

		int cutoff30 = ComputeCutoffStamp(30);
		int cutoffToday = ComputeCutoffTodayStamp();

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

	// --- Timestamp helpers ---

	// Returns YYYYMMDDhhmmss for (now - daysBack days).
	protected int ComputeCutoffStamp(int daysBack)
	{
		int y;
		int mo;
		int d;
		int h;
		int mn;
		int s;
		GetYearMonthDayUTC(y, mo, d);
		GetHourMinuteSecondUTC(h, mn, s);

		d = d - daysBack;
		while (d < 1)
		{
			mo = mo - 1;
			if (mo < 1)
			{
				mo = 12;
				y = y - 1;
			}
			d = d + DaysInMonth(y, mo);
		}
		return y * 10000000000 + mo * 100000000 + d * 1000000 + h * 10000 + mn * 100 + s;
	}

	// Returns YYYYMMDDhhmmss for start-of-today (midnight UTC).
	protected int ComputeCutoffTodayStamp()
	{
		int y;
		int mo;
		int d;
		GetYearMonthDayUTC(y, mo, d);
		return y * 10000000000 + mo * 100000000 + d * 1000000;
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

	// --- Event Handlers for Templates ---

	// Called when user clicks action button on an alert row
	// The _inputAlert variable is set by the template binding
	void OnAlertActionClicked()
	{
		if (!_inputAlert)
		{
			DZLog().LogWarning("[TXB] Alert action clicked but _inputAlert is null");
			return;
		}

		DZLog().LogInfo("[TXB] Alert action clicked: " + _inputAlert.title + " -> " + _inputAlert.actionTarget);

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
		else if (_inputAlert.actionTarget == "banking-clan-tab")
		{
			// Navigate to clan tab
			SelectComponent("banking-clan-tab");
		}
		else if (_inputAlert.actionTarget == "accept_invite")
		{
			// Handle clan invite acceptance
			// TODO: Call TXBClanService to accept invite
			DZLog().LogInfo("[TXB] Accepting clan invite from alert");
		}
		else if (_inputAlert.actionTarget == "dismiss")
		{
			// Dismiss this alert
			DismissAlert(_inputAlert.alertId);
		}
		else
		{
			DZLog().LogWarning("[TXB] Unknown alert action target: " + _inputAlert.actionTarget);
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
				DZLog().LogInfo("[TXB] Alert dismissed: " + alertId);
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
	ref TXBClanInvite _inputInvite;
}
