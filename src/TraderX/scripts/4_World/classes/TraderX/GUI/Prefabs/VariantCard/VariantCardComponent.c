class VariantCardComponent extends DZlrComponentBase
{
    private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("traderx-variant-card", VariantCardComponent);
    static ref ScriptInvoker Event_OnVariantClickCallBack = new ScriptInvoker();

    ref TraderXProduct item;
    string variant_classname, variant_price;
    EntityAI preview;
    TextWidget variantClassnameText, variantPriceText;
    ItemPreviewWidget variantPreview;
    ButtonWidget variantBtn;

    void VariantCardComponent(TraderXProduct item) { this.item = item; }

    override protected void RegisterMetadata()
    {
        m_Metadata = new ComponentMetadata();
        m_Metadata.selector = "traderx-variant-card";
        m_Metadata.templatePath = "TraderX/datasets/gui/TraderXPrefab/Variant/VariantCardView.layout";
    }

    override void OnInit()
    {
        variant_classname = item.GetDisplayName();
        variant_price = TraderXQuantityManager.GetFormattedMoneyAmount(item.buyPrice);
        preview = EntityAI.Cast(GetGame().CreateObjectEx(item.className, vector.Zero, ECE_LOCAL | ECE_NOLIFETIME));
        if (variantClassnameText) variantClassnameText.SetText(variant_classname);
        if (variantPriceText) variantPriceText.SetText(variant_price);
        if (variantPreview) variantPreview.SetItem(preview);
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (w == variantBtn) { Event_OnVariantClickCallBack.Invoke(item); return true; }
        return false;
    }
}
