class BuyTooltipComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("tx-buy-tooltip", BuyTooltipComponent);

	string product_description;

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "tx-buy-tooltip";
		m_Metadata.templatePath = "TraderX/datasets/gui/TraderXPrefab/Tooltip/BuyTooltip.layout";
	}

	override void OnInit()
	{
		AutoLoadWidgets();
	}

	// Plain description reveal. Variants/attachments state and the interaction hints used
	// to live here; they now live on the card as persistent icons (UpdateCustomizeAffordance).
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
