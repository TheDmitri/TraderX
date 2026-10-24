// TraderXBanking - Transfer Tab Component
// Backs the Transfer tab (TransferTabView.layout).
// Drives recipient/amount/note form, live fee summary and the transfer RPC.

class TXBTransferTabComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("banking-transfer-tab", TXBTransferTabComponent);

	// Form (two-way bindings)
	string destinationAccountId;
	string transferAmount;
	string transferNote;
	string contactsSearch;

	// Summary/display
	string transferFeesDisplay;
	string transferFeeLabel;
	string transferBalanceText;
	string transferStatusText;

	// Contact row labels (up to 5 recent recipients)
	string contact0Label;
	string contact1Label;
	string contact2Label;
	string contact3Label;
	string contact4Label;
	Widget contactRow0;
	Widget contactRow1;
	Widget contactRow2;
	Widget contactRow3;
	Widget contactRow4;
	Widget contactsEmpty;
	protected ref array<ref TXBContact> m_FilteredContacts = new array<ref TXBContact>();

	// Internal state
	protected int m_BankBalance;
	protected string m_PlayerUID;
	protected float m_FeePercent;
	protected float m_ThrottleTimer;
	protected bool m_TransferPending;
	protected float m_PendingTimer;
	protected const float PENDING_TIMEOUT = 15.0;
	protected const float THROTTLE_INTERVAL = 1.0;

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "banking-transfer-tab";
		m_Metadata.templatePath = "TraderXBanking/datasets/layouts/TransferTabView.layout";
	}

	override void OnInit()
	{
		AutoLoadWidgets();
		PlayerIdentity identity = GetGame().GetPlayer().GetIdentity();
		if (identity)
			m_PlayerUID = identity.GetPlainId();

		TXBSettings clientSettings = GetTXBConfigService().GetClientSettings();
		if (clientSettings)
			m_FeePercent = clientSettings.transactionFeePercent;

		transferFeeLabel = BuildFeeLabel();
		ResetForm();

		TXBBankingService.Event_OnAccountUpdated.Insert(OnAccountUpdated);
		TXBBankingService.Event_OnContactsUpdated.Insert(OnContactsUpdated);
		TXBBankingService.Event_OnTransactionResult.Insert(OnTransactionResult);

		GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(PeriodicUpdate, 500, true);

		RefreshFromAccount();
		RefreshContactsList();
		MarkForCheck();
	}

	override void OnDestroy()
	{
		TXBBankingService.Event_OnAccountUpdated.Remove(OnAccountUpdated);
		TXBBankingService.Event_OnContactsUpdated.Remove(OnContactsUpdated);
		TXBBankingService.Event_OnTransactionResult.Remove(OnTransactionResult);
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(PeriodicUpdate);
	}

	void OnContactsUpdated()
	{
		RefreshContactsList();
		MarkForCheck();
	}

	void OnContactsSearchChanged()
	{
		RefreshContactsList();
		MarkForCheck();
	}

	protected void RefreshContactsList()
	{
		m_FilteredContacts.Clear();
		array<ref TXBContact> allContacts = GetTXBBankingService().GetClientContacts();
		string needle = contactsSearch;
		needle.TrimInPlace();
		needle.ToLower();

		if (allContacts)
		{
			for (int i = allContacts.Count() - 1; i >= 0; i--)
			{
				TXBContact c = allContacts[i];
				if (!c || c.uid == "") continue;
				if (needle != "")
				{
					string hay = c.name + " " + c.uid;
					hay.ToLower();
					if (!hay.Contains(needle))
						continue;
				}
				m_FilteredContacts.Insert(c);
				if (m_FilteredContacts.Count() >= 5)
					break;
			}
		}

		contact0Label = FormatContactRow(0, contactRow0);
		contact1Label = FormatContactRow(1, contactRow1);
		contact2Label = FormatContactRow(2, contactRow2);
		contact3Label = FormatContactRow(3, contactRow3);
		contact4Label = FormatContactRow(4, contactRow4);

		if (contactsEmpty)
			contactsEmpty.Show(m_FilteredContacts.Count() == 0);
	}

	protected string FormatContactRow(int idx, Widget rowWidget)
	{
		if (idx < 0 || idx >= m_FilteredContacts.Count())
		{
			if (rowWidget) rowWidget.Show(false);
			return "";
		}
		TXBContact c = m_FilteredContacts[idx];
		if (rowWidget) rowWidget.Show(true);
		string displayName = c.name;
		if (displayName == "")
			displayName = "Recipient";
		return displayName + "  —  " + c.uid;
	}

	protected void SelectContactAt(int idx)
	{
		if (idx < 0 || idx >= m_FilteredContacts.Count())
			return;
		destinationAccountId = m_FilteredContacts[idx].uid;
		MarkForCheck();
	}

	void OnContact0() { SelectContactAt(0); }
	void OnContact1() { SelectContactAt(1); }
	void OnContact2() { SelectContactAt(2); }
	void OnContact3() { SelectContactAt(3); }
	void OnContact4() { SelectContactAt(4); }

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
			if (m_PlayerUID == "" && account.playerUID != "")
				m_PlayerUID = account.playerUID;
		}
	}

	void PeriodicUpdate()
	{
		if (m_ThrottleTimer > 0)
			m_ThrottleTimer -= 0.5;

		// Safety net: never leave the UI stuck in "in progress" if no answer arrives.
		if (m_TransferPending)
		{
			m_PendingTimer -= 0.5;
			if (m_PendingTimer <= 0)
			{
				m_TransferPending = false;
				SetStatus("No response from the bank server. Check your balance before retrying.", false);
			}
		}
	}

	// Server reported the outcome of a transaction.
	void OnTransactionResult(int txType, bool success, string message)
	{
		if (txType != TXBConstants.TRANSACTION_TRANSFER)
			return;
		if (!m_TransferPending)
			return;

		m_TransferPending = false;
		SetStatus(message, success);
		if (success)
		{
			transferAmount = "";
			transferNote = "";
			OnTransferAmountChanged();
		}
	}

	protected string BuildFeeLabel()
	{
		float pct = m_FeePercent * 100.0;
		int pctInt = Math.Round(pct);
		string pctText = pctInt.ToString();
		if (Math.AbsFloat(pct - pctInt) > 0.01)
			pctText = pct.ToString();
		return "Transfer fee - " + pctText + "%";
	}

	// Own UID as known by the client (identity first, server-synced account as fallback).
	protected bool IsOwnAccount(string uid)
	{
		if (m_PlayerUID != "" && uid == m_PlayerUID)
			return true;
		TXBAccountData account = GetTXBBankingService().GetClientAccount();
		if (account && account.playerUID != "" && uid == account.playerUID)
			return true;
		return false;
	}

	// --- Form actions ---

	void OnTransfer()
	{
		if (m_ThrottleTimer > 0)
		{
			SetStatus("Please wait...", false);
			return;
		}

		if (m_TransferPending)
		{
			SetStatus("A transfer is already in progress...", false);
			return;
		}

		// Refresh fee percent in case settings arrived after the tab opened.
		TXBSettings clientSettings = GetTXBConfigService().GetClientSettings();
		if (clientSettings)
			m_FeePercent = clientSettings.transactionFeePercent;

		int amount = transferAmount.ToInt();
		if (amount <= 0)
		{
			SetStatus("Enter a valid amount.", false);
			return;
		}

		string destUID = destinationAccountId;
		destUID.TrimInPlace();
		if (destUID.Length() < 16)
		{
			SetStatus("Enter a valid recipient account ID.", false);
			return;
		}

		if (IsOwnAccount(destUID))
		{
			SetStatus("Cannot transfer to your own account.", false);
			return;
		}

		int fees = Math.Round(amount * m_FeePercent);
		int total = amount + fees;
		if (total > m_BankBalance)
		{
			SetStatus("Insufficient balance: " + amount.ToString() + " + " + fees.ToString() + " fee = " + total.ToString() + ".", false);
			return;
		}

		GetDayZularRPCManager().SendRPC(TXBConstants.RPC_NAMESPACE, "RequestTransfer", new Param3<string, int, string>(destUID, amount, transferNote), true, null);

		SetStatus("Transferring " + amount.ToString() + " to " + destUID + " (fee " + fees.ToString() + ", total " + total.ToString() + ")...", true);
		m_ThrottleTimer = THROTTLE_INTERVAL;
		m_TransferPending = true;
		m_PendingTimer = PENDING_TIMEOUT;
	}

	void OnTransferAmountChanged()
	{
		TXBSettings clientSettings = GetTXBConfigService().GetClientSettings();
		if (clientSettings)
			m_FeePercent = clientSettings.transactionFeePercent;
		transferFeeLabel = BuildFeeLabel();

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
