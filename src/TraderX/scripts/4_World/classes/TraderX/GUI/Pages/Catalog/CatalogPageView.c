class CatalogPageView: ScriptViewTemplate<CatalogPageViewController>
{
	void CatalogPageView()
	{
		m_TemplateController.Setup();
	}

    override string GetLayoutFile() 
	{
		return "TraderX/datasets/gui/CatalogPage/CatalogPageView.layout";
	}
}
