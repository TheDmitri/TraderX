class BuyTooltipViewController: ViewController
{
    string variants_status;
    string attachments_status;

    string product_description;

    TextWidget variantsText;
    TextWidget attachmentsText;

    void Setup(string productDescription, bool hasVariants, bool hasAttachments)
    {
        if(!hasVariants)
        {
            this.variants_status = "#tpm_none";
            variantsText.SetColor(TraderXViewStyles.TEXT_WARNING);
        }
        else
        {
            this.variants_status = "#tpm_available";
            variantsText.SetColor(TraderXViewStyles.TEXT_GREEN);
        }

        if(!hasAttachments)
        {
            this.attachments_status = "#tpm_none";
            attachmentsText.SetColor(TraderXViewStyles.TEXT_WARNING);
        }
        else
        {
            this.attachments_status = "#tpm_available";
            attachmentsText.SetColor(TraderXViewStyles.TEXT_GREEN);
        }
        
        this.product_description = productDescription;
        NotifyPropertiesChanged({"variants_status", "attachments_status", "product_description"});
    }
}