// TraderXBanking - Banking UI Component
// DayZular component handling all banking UI interactions

class TXBBankingComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("txb-banking", TXBBankingComponent);

	// Bound properties (updated via MarkForCheck)
	string feesText;
	string bankBalanceText;
	string playerCashText;
	string playerIdText;
	string transferBalanceText;
	string statusText;
	int progressValue;

	// Internal state
	protected int m_BankBalance;
	protected int m_MaxBalance;
	protected int m_PlayerCash;
	protected string m_PlayerUID;
	protected float m_FeePercent;
	protected float m_ThrottleTimer;
	protected float m_ThrottleInterval = 1.0;

	// Panel widgets for tab switching
	Widget PlayerAccountPanel;
	Widget TransferPanel;

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "txb-banking";
		m_Metadata.templatePath = "TraderXBanking/datasets/layouts/TXBBankingComponent.layout";
	}

	override void OnInit()
	{
		DZLog().LogDebug("[TXB] BankingComponent OnInit");

		// Get player ID
		PlayerIdentity identity = GetGame().GetPlayer().GetIdentity();
		if (identity)
			m_PlayerUID = identity.GetPlainId();

		// Load fee from client settings
		TXBSettings clientSettings = GetTXBConfigService().GetClientSettings();
		if (clientSettings)
			m_FeePercent = clientSettings.transactionFeePercent;

		feesText = "Fee: " + (m_FeePercent * 100).ToString() + "%";
		playerIdText = m_PlayerUID;
		statusText = "";

		// Subscribe to account updates
		TXBBankingService.Event_OnAccountUpdated.Insert(OnAccountUpdated);

		// Start periodic cash update
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(PeriodicUpdate, 500, true);

		// Load current account state
		RefreshFromAccount();
		MarkForCheck();
	}

	override void OnDestroy()
	{
		TXBBankingService.Event_OnAccountUpdated.Remove(OnAccountUpdated);
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(PeriodicUpdate);
	}

	// --- Account Data Sync ---

	void OnAccountUpdated()
	{
		RefreshFromAccount();
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

		UpdateDisplayTexts();
	}

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
		if (!player)
			return;

		TXBSettings settings = GetTXBConfigService().GetClientSettings();
		if (!settings)
			return;

		m_PlayerCash = GetTXBBankingService().GetPlayerMoney(player, settings);
		playerCashText = m_PlayerCash.ToString();
	}

	protected void UpdateDisplayTexts()
	{
		bankBalanceText = m_BankBalance.ToString() + " / " + m_MaxBalance.ToString();
		transferBalanceText = m_BankBalance.ToString();

		if (m_MaxBalance > 0)
			progressValue = Math.Round((m_BankBalance * 100) / m_MaxBalance);
		else
			progressValue = 0;
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

	protected void ResetThrottle()
	{
		m_ThrottleTimer = m_ThrottleInterval;
	}

	// --- Tab Switching ---

	void OnAccountTabClicked()
	{
		if (PlayerAccountPanel)
			PlayerAccountPanel.Show(true);
		if (TransferPanel)
			TransferPanel.Show(false);

		RefreshFromAccount();
		MarkForCheck();
	}

	void OnTransferTabClicked()
	{
		if (PlayerAccountPanel)
			PlayerAccountPanel.Show(false);
		if (TransferPanel)
			TransferPanel.Show(true);

		RefreshFromAccount();
		MarkForCheck();
	}

	// --- Deposit ---

	void OnDepositClicked()
	{
		if (!CanTransact())
			return;

		Widget editWidget = GetRoot().FindAnyWidget("DepositAmountInput");
		if (!editWidget)
			return;

		EditBoxWidget editBox = EditBoxWidget.Cast(editWidget);
		if (!editBox)
			return;

		string inputText = editBox.GetText();
		int amount = inputText.ToInt();

		if (amount <= 0)
		{
			statusText = "Enter a valid amount";
			MarkForCheck();
			return;
		}

		if (amount > m_PlayerCash)
		{
			statusText = "Not enough cash in inventory";
			MarkForCheck();
			return;
		}

		int afterFees = amount - Math.Round(amount * m_FeePercent);
		if (m_BankBalance + afterFees > m_MaxBalance)
		{
			statusText = "Would exceed max balance";
			MarkForCheck();
			return;
		}

		GetDayZularRPCManager().SendRPC(TXB_RPC_NAMESPACE, "RequestDeposit", new Param1<int>(amount), true, null);

		statusText = "Depositing " + amount.ToString() + "...";
		ResetThrottle();
		MarkForCheck();
	}

	void OnDepositAmountChanged()
	{
		Widget editWidget = GetRoot().FindAnyWidget("DepositAmountInput");
		if (!editWidget)
			return;

		EditBoxWidget editBox = EditBoxWidget.Cast(editWidget);
		if (!editBox)
			return;

		int amount = editBox.GetText().ToInt();
		if (amount > 0)
		{
			int afterFees = amount - Math.Round(amount * m_FeePercent);
			bankBalanceText = m_BankBalance.ToString() + " + " + afterFees.ToString() + " / " + m_MaxBalance.ToString();
		}
		else
		{
			bankBalanceText = m_BankBalance.ToString() + " / " + m_MaxBalance.ToString();
		}

		MarkForCheck();
	}

	// --- Withdraw ---

	void OnWithdrawClicked()
	{
		if (!CanTransact())
			return;

		Widget editWidget = GetRoot().FindAnyWidget("DepositAmountInput");
		if (!editWidget)
			return;

		EditBoxWidget editBox = EditBoxWidget.Cast(editWidget);
		if (!editBox)
			return;

		int amount = editBox.GetText().ToInt();

		if (amount <= 0)
		{
			statusText = "Enter a valid amount";
			MarkForCheck();
			return;
		}

		if (amount > m_BankBalance)
		{
			statusText = "Insufficient bank balance";
			MarkForCheck();
			return;
		}

		GetDayZularRPCManager().SendRPC(TXB_RPC_NAMESPACE, "RequestWithdraw", new Param1<int>(amount), true, null);

		statusText = "Withdrawing " + amount.ToString() + "...";
		ResetThrottle();
		MarkForCheck();
	}

	// --- Transfer ---

	void OnTransferClicked()
	{
		if (!CanTransact())
			return;

		Widget amountWidget = GetRoot().FindAnyWidget("TransferAmountInput");
		Widget idWidget = GetRoot().FindAnyWidget("TransferIdInput");
		if (!amountWidget || !idWidget)
			return;

		EditBoxWidget amountBox = EditBoxWidget.Cast(amountWidget);
		EditBoxWidget idBox = EditBoxWidget.Cast(idWidget);
		if (!amountBox || !idBox)
			return;

		int amount = amountBox.GetText().ToInt();
		string destUID = idBox.GetText();

		if (amount <= 0)
		{
			statusText = "Enter a valid amount";
			MarkForCheck();
			return;
		}

		if (destUID.Length() < 16)
		{
			statusText = "Enter a valid account ID";
			MarkForCheck();
			return;
		}

		if (destUID == m_PlayerUID)
		{
			statusText = "Cannot transfer to yourself";
			MarkForCheck();
			return;
		}

		int fees = Math.Round(amount * m_FeePercent);
		if (amount + fees > m_BankBalance)
		{
			statusText = "Insufficient balance (including fees)";
			MarkForCheck();
			return;
		}

		GetDayZularRPCManager().SendRPC(TXB_RPC_NAMESPACE, "RequestTransfer", new Param2<string, int>(destUID, amount), true, null);

		statusText = "Transferring " + amount.ToString() + "...";
		ResetThrottle();
		MarkForCheck();
	}

	void OnTransferAmountChanged()
	{
		Widget amountWidget = GetRoot().FindAnyWidget("TransferAmountInput");
		if (!amountWidget)
			return;

		EditBoxWidget amountBox = EditBoxWidget.Cast(amountWidget);
		if (!amountBox)
			return;

		int amount = amountBox.GetText().ToInt();
		if (amount > 0)
		{
			int fees = Math.Round(amount * m_FeePercent);
			transferBalanceText = m_BankBalance.ToString() + " - " + (amount + fees).ToString();
		}
		else
		{
			transferBalanceText = m_BankBalance.ToString();
		}

		MarkForCheck();
	}

	// --- Copy ID ---

	void OnCopyIdClicked()
	{
		GetGame().CopyToClipboard(m_PlayerUID);
		statusText = "Account ID copied to clipboard";
		MarkForCheck();
	}
}
