class SellTooltipComponent extends DZlrComponentBase
{
    private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("traderx-sell-tooltip", SellTooltipComponent);

    string m_Description, m_Hierarchy;
    float m_OffsetX, m_OffsetY;

    TextWidget productDescriptionText, hierarchyText;
    Widget leftRightClickPanel, quickSellPanel, shiftLeftClickPanel, shiftRightPanel, manualModePanel;

    void SellTooltipComponent(string description, string hierarchy, float offsetX, float offsetY)
    {
        m_Description = description;
        m_Hierarchy = hierarchy;
        m_OffsetX = offsetX;
        m_OffsetY = offsetY;
    }

    override protected void RegisterMetadata()
    {
        m_Metadata = new ComponentMetadata();
        m_Metadata.selector = "traderx-sell-tooltip";
        m_Metadata.templatePath = "TraderX/datasets/gui/TraderXPrefab/Tooltip/SellTooltip.layout";
    }

    override void OnInit()
    {
        if (productDescriptionText) productDescriptionText.SetText(m_Description);
        if (hierarchyText) hierarchyText.SetText(m_Hierarchy);

        if (!SellPageComponent.IsManualSell())
        {
            if (leftRightClickPanel) leftRightClickPanel.Show(true);
            if (shiftLeftClickPanel) shiftLeftClickPanel.Show(true);
            if (shiftRightPanel) shiftRightPanel.Show(true);
            if (manualModePanel) manualModePanel.Show(false);
            if (quickSellPanel) quickSellPanel.Show(false);
        }
        else
        {
            if (leftRightClickPanel) leftRightClickPanel.Show(false);
            if (shiftLeftClickPanel) shiftLeftClickPanel.Show(false);
            if (shiftRightPanel) shiftRightPanel.Show(false);
            if (manualModePanel) manualModePanel.Show(true);
            if (quickSellPanel) quickSellPanel.Show(true);
        }

        int x, y;
        GetMousePos(x, y);
        if (m_Root) { m_Root.SetPos(x + 10.0, y - 10.0, true); m_Root.Update(); }
    }
}
