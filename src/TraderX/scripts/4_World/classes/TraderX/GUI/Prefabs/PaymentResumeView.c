class PaymentResumeView: ScriptViewTemplate<PaymentResumeViewController>
{
    override string GetLayoutFile() 
	{
		return "TraderX/datasets/gui/TraderXPrefab/PaymentResumeView.layout";
	}

    override bool OnMouseEnter(Widget w, int x, int y)
	{
        if(w && w.GetUserID() == EButtonIds.PURCHASE){
            //Button over
        }
        return false;
	}

    override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
        if(w && w.GetUserID() == EButtonIds.PURCHASE){
            //Button out
        }
        return false;
	}
}