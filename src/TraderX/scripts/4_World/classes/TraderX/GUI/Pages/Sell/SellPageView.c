class SellPageView: ScriptViewTemplate<SellPageViewController>
{
	void SellPageView()
	{
		m_TemplateController.Setup();
	}

    override string GetLayoutFile() 
	{
		return "TraderX/datasets/gui/SellPage/SellPageView.layout";
	}
}