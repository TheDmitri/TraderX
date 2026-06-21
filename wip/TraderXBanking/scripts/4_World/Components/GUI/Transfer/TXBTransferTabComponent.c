// TraderXBanking - Transfer Tab Component
// Backs the Transfer tab (TransferTabView.layout).
// Drives recipient/amount/note form, live fee summary and the transfer RPC.

class TXBTransferTabComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register =
		new ComponentAutoRegister("banking-transfer-tab", TXBTransferTabComponent);

	// Form (two-way bindings)
	string destinationAccountId;
	string transferAmount;
	string transferNote;
	string contactsSearch;

	// Summary/display
	string transferFeesDisplay;
	string transferBalanceText;
	string transferStatusText;

	// Internal state
	protected int m_BankBalance;
	protected string m_PlayerUID;
	protected float m_FeePercent;
	protected float m_ThrottleTimer;
	protected const float THROTTLE_INTERVAL = 1.0;

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "banking-transfer-tab";
		m_Metadata.templatePath = "TraderXBanking/datasets/layouts/TransferTabView.layout";
	}

	override void OnInit()
	{
		PlayerIdentity identity = GetGame().GetPlayer().GetIdentity();
		if (identity)
			m_PlayerUID = identity.GetPlainId();

		TXBSettings clientSettings = GetTXBConfigService().GetClientSettings();
		if (clientSettings)
			m_FeePercent = clientSettings.transactionFeePercent;

		ResetForm();

		TXBBankingService.Event_OnAccountUpdated.Insert(OnAccountUpdated);

		GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(PeriodicUpdate, 500, true);

		RefreshFromAccount();
		MarkForCheck();
	}

	override void OnDestroy()
	{
		TXBBankingService.Event_OnAccountUpdated.Remove(OnAccountUpdated);
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(PeriodicUpdate);
	}

	void OnAccountUpdated()
	{
		RefreshFromAccount();
		MarkForCheck();
	}

	protected void RefreshFromAccount()
	{
		TXBAccountData account = GetTXBBankingService().GetClientAccount();
		if (account)
			m_BankBalance = account.balance;
	}

	void PeriodicUpdate()
	{
		if (m_ThrottleTimer > 0)
			m_ThrottleTimer -= 0.5;
	}

	// --- Form actions ---

	void OnTransfer()
	{
		if (m_ThrottleTimer > 0)
		{
			SetStatus("Please wait...", false);
			return;
		}

		int amount = transferAmount.ToInt();
		if (amount <= 0)
		{
			SetStatus("Enter a valid amount.", false);
			return;
		}

		if (destinationAccountId.Length() < 16)
		{
			SetStatus("Enter a valid recipient account ID.", false);
			return;
		}

		if (destinationAccountId == m_PlayerUID)
		{
			SetStatus("Cannot transfer to your own account.", false);
			return;
		}

		int fees = Math.Round(amount * m_FeePercent);
		if (amount + fees > m_BankBalance)
		{
			SetStatus("Insufficient balance (amount + fees).", false);
			return;
		}

		GetDayZularRPCManager().SendRPC(TXBConstants.RPC_NAMESPACE, "RequestTransfer", new Param3<string, int, string>(destinationAccountId, amount, transferNote), true, null);

		SetStatus("Transferring " + amount.ToString() + " to " + destinationAccountId + "...", true);
		m_ThrottleTimer = THROTTLE_INTERVAL;
	}

	void OnTransferAmountChanged()
	{
		int amount = transferAmount.ToInt();
		if (amount > 0)
		{
			int fees = Math.Round(amount * m_FeePercent);
			transferFeesDisplay = fees.ToString();
			transferBalanceText = (amount + fees).ToString();
		}
		else
		{
			transferFeesDisplay = "0";
			transferBalanceText = "0";
		}
		MarkForCheck();
	}

	void OnReset()
	{
		ResetForm();
		MarkForCheck();
	}

	protected void ResetForm()
	{
		destinationAccountId = "";
		transferAmount = "";
		transferNote = "";
		transferFeesDisplay = "0";
		transferBalanceText = "0";
		contactsSearch = "";
		transferStatusText = "";
		HideStatusPanel();
	}

	// --- Status panel ---

	protected void SetStatus(string message, bool success)
	{
		transferStatusText = message;
		ShowStatusPanel();
		MarkForCheck();
	}

	protected void ShowStatusPanel()
	{
		Widget panel = null;
		Widget root = GetRoot();
		if (root) panel = root.FindAnyWidget("transferStatusPanel");
		if (panel) panel.Show(true);
	}

	protected void HideStatusPanel()
	{
		Widget panel = null;
		Widget root = GetRoot();
		if (root) panel = root.FindAnyWidget("transferStatusPanel");
		if (panel) panel.Show(false);
	}
}
