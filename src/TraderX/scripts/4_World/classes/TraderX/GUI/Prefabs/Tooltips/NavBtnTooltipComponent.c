class NavBtnTooltipComponent extends DZlrComponentBase
{
    private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("traderx-navbtn-tooltip", NavBtnTooltipComponent);

    string m_Title;
    float m_OffsetX, m_OffsetY;
    TextWidget titleText;

    void NavBtnTooltipComponent(string title, float offsetX, float offsetY)
    {
        m_Title = title;
        m_OffsetX = offsetX;
        m_OffsetY = offsetY;
    }

    override protected void RegisterMetadata()
    {
        m_Metadata = new ComponentMetadata();
        m_Metadata.selector = "traderx-navbtn-tooltip";
        m_Metadata.templatePath = "TraderX/datasets/gui/TraderXPrefab/Tooltip/NavBtnTooltip.layout";
    }

    override void OnInit()
    {
        if (titleText) titleText.SetText(m_Title);
        int x, y;
        GetMousePos(x, y);
        if (m_Root) { m_Root.SetPos(x + m_OffsetX, y + m_OffsetY, true); m_Root.Update(); }
    }
}
