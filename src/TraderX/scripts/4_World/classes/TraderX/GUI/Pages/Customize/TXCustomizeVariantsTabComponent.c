// Customize variants tab. Renders the product's variants as tx-variant-card via
// Component_ForEach. Clicking a variant routes through CustomizeStateService.
class TXCustomizeVariantsTabComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("tx-customize-variants", TXCustomizeVariantsTabComponent);

	ref array<ref TraderXProduct> _inputVariants = new array<ref TraderXProduct>();
	ref TraderXProduct originalItem;
	string empty_label;

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "tx-customize-variants";
		m_Metadata.templatePath = "TraderX/datasets/gui/PurchasePage/Customize/CustomizeVariantsTabView.layout";
	}

	override void OnInit()
	{
		originalItem = CustomizeStateService.GetInstance().GetOriginalItem();
		VariantCardComponent.Event_OnVariantClickCallBack.Insert(OnVariantClick);
		FillVariantList();
	}

	override void OnDestroy()
	{
		VariantCardComponent.Event_OnVariantClickCallBack.Remove(OnVariantClick);
	}

	void FillVariantList()
	{
		if (!originalItem) return;
		_inputVariants.Clear();
		array<ref TraderXProduct> variants = originalItem.GetVariants();
		foreach (TraderXProduct variant : variants)
			_inputVariants.Insert(variant);
		empty_label = TernaryString(_inputVariants.Count() == 0, "#tpm_no_variants", "");
		MarkForCheck();
	}

	void OnVariantClick(TraderXProduct item)
	{
		CustomizeStateService.GetInstance().SetCurrentVariant(item);
	}
}
