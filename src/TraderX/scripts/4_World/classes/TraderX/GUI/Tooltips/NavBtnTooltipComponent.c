class NavBtnTooltipComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("tx-nav-tooltip", NavBtnTooltipComponent);

	string title;

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "tx-nav-tooltip";
		m_Metadata.templatePath = "TraderX/datasets/gui/TraderXPrefab/Tooltip/NavBtnTooltip.layout";
	}

	void Setup(string tooltipTitle)
	{
		title = tooltipTitle;
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
