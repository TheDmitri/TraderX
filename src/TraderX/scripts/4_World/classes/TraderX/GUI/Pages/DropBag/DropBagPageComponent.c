class DropBagPageComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("tx-dropbag-page", DropBagPageComponent);

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "tx-dropbag-page";
		m_Metadata.templatePath = "TraderX/datasets/gui/DropBag/DropBagView.layout";
	}
}
