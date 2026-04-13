class ComboBoxPrefabComponent extends DZlrComponentBase
{
    private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("traderx-combobox", ComboBoxPrefabComponent);

    override protected void RegisterMetadata()
    {
        m_Metadata = new ComponentMetadata();
        m_Metadata.selector = "traderx-combobox";
        m_Metadata.templatePath = "TraderX/datasets/gui/TraderXPrefab/ComboBoxPrefab.layout";
    }
}
