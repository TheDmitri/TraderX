class BuyTooltipComponent extends DZlrComponentBase
{
    private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("traderx-buy-tooltip", BuyTooltipComponent);

    string m_Description;
    bool m_HasVariants, m_HasAttachments;
    float m_OffsetX, m_OffsetY;

    TextWidget variantsText, attachmentsText, productDescriptionText;

    void BuyTooltipComponent(string description, bool hasVariants, bool hasAttachments, float offsetX, float offsetY)
    {
        m_Description = description;
        m_HasVariants = hasVariants;
        m_HasAttachments = hasAttachments;
        m_OffsetX = offsetX;
        m_OffsetY = offsetY;
    }

    override protected void RegisterMetadata()
    {
        m_Metadata = new ComponentMetadata();
        m_Metadata.selector = "traderx-buy-tooltip";
        m_Metadata.templatePath = "TraderX/datasets/gui/TraderXPrefab/Tooltip/BuyTooltip.layout";
    }

    override void OnInit()
    {
        if (!m_HasVariants)
        {
            if (variantsText) { variantsText.SetText("#tpm_none"); variantsText.SetColor(TraderXViewStyles.TEXT_WARNING); }
        }
        else
        {
            if (variantsText) { variantsText.SetText("#tpm_available"); variantsText.SetColor(TraderXViewStyles.TEXT_GREEN); }
        }

        if (!m_HasAttachments)
        {
            if (attachmentsText) { attachmentsText.SetText("#tpm_none"); attachmentsText.SetColor(TraderXViewStyles.TEXT_WARNING); }
        }
        else
        {
            if (attachmentsText) { attachmentsText.SetText("#tpm_available"); attachmentsText.SetColor(TraderXViewStyles.TEXT_GREEN); }
        }

        if (productDescriptionText) productDescriptionText.SetText(m_Description);

        int x, y;
        GetMousePos(x, y);
        if (m_Root) { m_Root.SetPos(x + 10.0, y - 10.0, true); m_Root.Update(); }
    }
}
