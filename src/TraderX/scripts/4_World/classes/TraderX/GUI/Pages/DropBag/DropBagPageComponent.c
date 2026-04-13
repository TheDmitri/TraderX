class DropBagPageComponent extends DZlrComponentBase
{
    private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("traderx-dropbag-page", DropBagPageComponent);

    override protected void RegisterMetadata()
    {
        m_Metadata = new ComponentMetadata();
        m_Metadata.selector = "traderx-dropbag-page";
        m_Metadata.templatePath = "";
    }
}
