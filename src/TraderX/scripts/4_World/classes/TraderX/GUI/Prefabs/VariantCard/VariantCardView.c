class VariantCardView: ScriptViewTemplate<VariantCardViewController>
{
    void VariantCardView(TraderXProduct item)
    {
        m_TemplateController.Setup(item);
    }
    override string GetLayoutFile() 
	{
		return "TraderX/datasets/gui/TraderXPrefab/Variant/VariantCardView.layout";
	}
}