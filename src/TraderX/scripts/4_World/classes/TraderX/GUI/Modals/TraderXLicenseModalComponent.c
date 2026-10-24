// Standalone modal shown when a player interacts with a license-locked trader.
// Lives in its own ComponentMenu (the trader view never opens), so closing it must
// free the component from outside the click event — deferred one frame to avoid the
// native crash that freeing a component mid-click causes.
class TraderXLicenseModalComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("tx-license-modal", TraderXLicenseModalComponent);

	string title;
	string content;
	string licensesList;

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "tx-license-modal";
		m_Metadata.templatePath = "TraderX/datasets/gui/TraderXPrefab/Modals/LicenseModalView.layout";
	}

	void Setup(array<string> requiredLicenses)
	{
		// Anton caps: translate, then uppercase (the uppercase pipe would only change the key)
		title = CheckoutComponent.Upper("#tpm_licenses_required");
		content = "You don't have the required license to access this trader.";
		licensesList = BuildLicensesList(requiredLicenses);
		MarkForCheck();
	}

	protected string BuildLicensesList(array<string> requiredLicenses)
	{
		string result = "";
		if (requiredLicenses)
		{
			for (int i = 0; i < requiredLicenses.Count(); i++)
			{
				if (i > 0)
					result = result + "\n";
				result = result + "- " + requiredLicenses[i];
			}
		}
		return result;
	}

	void OnCloseClick()
	{
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(DeferredClose, 1, false);
	}

	void DeferredClose()
	{
		if (GetTraderXUIService())
			GetTraderXUIService().CloseLicenseModal();
	}
}
