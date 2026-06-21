class UpgradePageComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("tx-upgrade-page", UpgradePageComponent);

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "tx-upgrade-page";
		m_Metadata.templatePath = "TraderX/datasets/gui/UpgradePage/UpgradePageView.layout";
	}
}
