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

		ResetForm();

		TXBBankingService.Event_OnAccountUpdated.Insert(OnAccountUpdated);
		TXBBankingService.Event_OnContactsUpdated.Insert(OnContactsUpdated);

		GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(PeriodicUpdate, 500, true);

		RefreshFromAccount();
		RefreshContactsList();
		MarkForCheck();
	}

	override void OnDestroy()
	{
		TXBBankingService.Event_OnAccountUpdated.Remove(OnAccountUpdated);
		TXBBankingService.Event_OnContactsUpdated.Remove(OnContactsUpdated);
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
