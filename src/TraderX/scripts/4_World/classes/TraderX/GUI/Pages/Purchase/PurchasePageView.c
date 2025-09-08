class PurchasePageView: ScriptViewTemplate<PurchasePageViewController>
{
	void PurchasePageView()
	{
		m_TemplateController.Setup();
	}

	override bool OnMouseButtonDown(Widget w, int x, int y, int button)
    {
      super.OnMouseButtonDown(w, x, y, button);
      PlayerPreviewController.GetInstance().OnMouseButtonDown(w, x, y, button);
      return false;
    }
	
    override string GetLayoutFile() 
	{
		return "TraderX/datasets/gui/PurchasePage/PurchasePageView.layout";
	}
}