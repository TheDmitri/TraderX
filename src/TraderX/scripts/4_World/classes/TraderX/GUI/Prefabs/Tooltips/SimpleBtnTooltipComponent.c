class SimpleBtnTooltipComponent extends DZlrComponentBase
{
    private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("traderx-simplebtn-tooltip", SimpleBtnTooltipComponent);

    string m_Title;
    float m_OffsetX, m_OffsetY;
    TextWidget titleText;

    void SimpleBtnTooltipComponent(string title, float offsetX, float offsetY)
    {
        m_Title = title;
        m_OffsetX = offsetX;
        m_OffsetY = offsetY;
    }

    override protected void RegisterMetadata()
    {
        m_Metadata = new ComponentMetadata();
        m_Metadata.selector = "traderx-simplebtn-tooltip";
        m_Metadata.templatePath = "TraderXCore/datasets/gui/Prefab/Tooltip/SimpleBtnTooltip.layout";
    }

    override void OnInit()
    {
        if (titleText) titleText.SetText(m_Title);
        int x, y;
        GetMousePos(x, y);
        if (m_Root) { m_Root.SetPos(x + m_OffsetX, y + m_OffsetY, true); m_Root.Update(); }
    }
}
