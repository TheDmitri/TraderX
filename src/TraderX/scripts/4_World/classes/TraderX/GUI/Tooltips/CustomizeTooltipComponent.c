class CustomizeTooltipComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("tx-customize-tooltip", CustomizeTooltipComponent);

	string product_description;

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "tx-customize-tooltip";
		m_Metadata.templatePath = "TraderX/datasets/gui/TraderXPrefab/Tooltip/CustomizeTooltip.layout";
	}

	void Setup(string productDescription)
	{
		product_description = productDescription;
		MarkForCheck();
		SetPositionTooltip();
	}

	void SetPositionTooltip()
	{
		int x, y;
		GetMousePos(x, y);
		GetRoot().SetPos(x + 10.0, y - 10.0, true);
		GetRoot().Update();
	}
}
