class SellTooltipComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("tx-sell-tooltip", SellTooltipComponent);

	string product_description;
	string hierarchy;

	Widget leftRightClickPanel;
	Widget quickSellPanel;
	Widget shiftLeftClickPanel;
	Widget shiftRightPanel;
	Widget manualModePanel;

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "tx-sell-tooltip";
		m_Metadata.templatePath = "TraderX/datasets/gui/TraderXPrefab/Tooltip/SellTooltip.layout";
	}

	override void OnInit()
	{
		AutoLoadWidgets();
	}

	void Setup(string productDescription, string productHierarchy)
	{
		product_description = productDescription;
		hierarchy = productHierarchy;

		if (!SellPageComponent.IsManualSell())
		{
			SelectionMode();
		}
		else
		{
			ManualMode();
		}

		MarkForCheck();
		SetPositionTooltip();
	}

	void ManualMode()
	{
		if (leftRightClickPanel) leftRightClickPanel.Show(false);
		if (shiftLeftClickPanel) shiftLeftClickPanel.Show(false);
		if (shiftRightPanel) shiftRightPanel.Show(false);
		if (manualModePanel) manualModePanel.Show(true);
		if (quickSellPanel) quickSellPanel.Show(true);
	}

	void SelectionMode()
	{
		if (leftRightClickPanel) leftRightClickPanel.Show(true);
		if (shiftLeftClickPanel) shiftLeftClickPanel.Show(true);
		if (shiftRightPanel) shiftRightPanel.Show(true);
		if (manualModePanel) manualModePanel.Show(false);
		if (quickSellPanel) quickSellPanel.Show(false);
	}

	void SetPositionTooltip()
	{
		int x, y;
		GetMousePos(x, y);
		GetRoot().SetPos(x + 10.0, y - 10.0, true);
		GetRoot().Update();
	}
}
