class CustomizePageView: ScriptViewTemplate<CustomizePageViewController>
{
	void CustomizePageView(TraderXProduct item)
	{
		m_TemplateController.Setup(item);
	}

    override string GetLayoutFile() 
	{
		return "TraderX/datasets/gui/PurchasePage/CustomizePageVariant.layout";
	}

	bool OnCloseCustomize(ButtonCommandArgs args)
    {
		Show(false);
		delete this;        
        return true;
    }
}