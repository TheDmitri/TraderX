class VariantCardComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("tx-variant-card", VariantCardComponent);

	static ref ScriptInvoker Event_OnVariantClickCallBack = new ScriptInvoker();

	ref TraderXProduct _inputProduct;

	string variant_classname;
	string variant_price;
	EntityAI preview;

	Widget selectedBadge;

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "tx-variant-card";
		m_Metadata.templatePath = "TraderX/datasets/gui/TraderXPrefab/Variant/VariantCardView.layout";
	}

	override void OnInit()
	{
		AutoLoadWidgets();
		CustomizeStateService.Event_OnVariantChanged.Insert(OnVariantChanged);
		if (_inputProduct)
			Setup(_inputProduct);
	}

	override void OnDestroy()
	{
		CustomizeStateService.Event_OnVariantChanged.Remove(OnVariantChanged);
	}

	void Setup(TraderXProduct item)
	{
		_inputProduct = item;
		variant_classname = item.GetDisplayName();
		variant_price = TraderXCurrencyService.GetInstance().GetTraderCurrencySymbol() + TraderXQuantityManager.GetFormattedMoneyAmount(item.buyPrice);
		preview = EntityAI.Cast(GetGame().CreateObjectEx(item.className, vector.Zero, ECE_LOCAL|ECE_NOLIFETIME));
		RefreshSelectedState();
		MarkForCheck();
	}

	void OnVariantChanged(TraderXProduct variant)
	{
		RefreshSelectedState();
	}

	// Highlight this card when its product is the active variant (or original).
	void RefreshSelectedState()
	{
		bool isSelected = false;
		TraderXProduct current = CustomizeStateService.GetInstance().GetCurrentVariant();
		if (_inputProduct && current && current.productId == _inputProduct.productId)
			isSelected = true;

		if (selectedBadge)
			selectedBadge.Show(isSelected);
	}

	void OnVariantClick()
	{
		if (_inputProduct)
			Event_OnVariantClickCallBack.Invoke(_inputProduct);
	}
}
