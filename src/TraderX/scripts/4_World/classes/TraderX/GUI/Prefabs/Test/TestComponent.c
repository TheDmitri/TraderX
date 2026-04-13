class TestComponent extends DZlrComponentBase
{
    private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("traderx-test", TestComponent);

    override protected void RegisterMetadata()
    {
        m_Metadata = new ComponentMetadata();
        m_Metadata.selector = "traderx-test";
        m_Metadata.templatePath = "TraderX/datasets/gui/TraderXPrefab/Panel/PanelShadowView.layout";
    }
}
