class CustomizeTooltipComponent extends DZlrComponentBase
{
    private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("traderx-customize-tooltip", CustomizeTooltipComponent);

    string m_Description;
    TextWidget productDescriptionText;

    void CustomizeTooltipComponent(string description)
    {
        m_Description = description;
    }

    override protected void RegisterMetadata()
    {
        m_Metadata = new ComponentMetadata();
        m_Metadata.selector = "traderx-customize-tooltip";
        m_Metadata.templatePath = "TraderX/datasets/gui/TraderXPrefab/Tooltip/CustomizeTooltip.layout";
    }

    override void OnInit()
    {
        if (productDescriptionText) productDescriptionText.SetText(m_Description);
        int x, y;
        GetMousePos(x, y);
        if (m_Root) { m_Root.SetPos(x + 10.0, y - 10.0, true); m_Root.Update(); }
    }
}
