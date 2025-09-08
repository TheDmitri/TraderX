class CheckoutCardView: ScriptViewTemplate<CheckoutCardViewController>
{
	TextWidget productPriceText;
	TextWidget productQtyText;
	TextWidget productNameText;

	void CheckoutCardView(TraderXProduct item){
		m_TemplateController.Setup(item);
	}

    override string GetLayoutFile() 
	{
		return "TraderX/datasets/gui/TraderXPrefab/Checkout/CheckoutCardView.layout";
	}
}