class ModalConfirmationView : ScriptViewTemplate<ModalConfirmationViewController>
{
    void ModalConfirmationView(string title, string content)
    {
        m_TemplateController.Setup(title, content);
    }

    override bool OnMouseEnter(Widget w, int x, int y)
	{
        if(w && w.GetUserID() == 333){
            w.SetAlpha(1.0);
        }

        return false;
	}

    override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
        if(w && w.GetUserID() == 333){
            w.SetAlpha(0.70);
        }
            
        return false;
	}

    override string GetLayoutFile() 
	{
		return "TraderX/datasets/gui/TraderXPrefab/Modals/ModalConfirmationView.layout";
	}
}