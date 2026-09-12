// One selectable preset row inside a checkout card's expandable preset list.
// Carries a back-reference to the owning checkout card so a click applies to the
// correct line (the preset list is a static-registered foreach; without the owner
// ref a global event would fire every checkout card at once).
class TXCheckoutPresetOption
{
	ref TraderXPreset preset;        // null for the "No preset" entry
	CheckoutCardComponent owner;
	string label;
	bool isNone;
}

// Renders a single preset option as a full-width button; on click it asks the owning
// checkout card to apply this option. Instantiated by the checkout card's ForEach.
class CheckoutPresetOptionComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("tx-checkout-preset-option", CheckoutPresetOptionComponent);

	ref TXCheckoutPresetOption _inputOption;
	protected ref TXCheckoutPresetOption m_BoundOption;

	string option_label;

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "tx-checkout-preset-option";
		m_Metadata.templatePath = "TraderX/datasets/gui/TraderXPrefab/Checkout/CheckoutPresetOptionView.layout";
	}

	override void OnInit()
	{
		AutoLoadWidgets();
		Refresh();
	}

	// Rebound by the ForEach when the list changes — refresh the label.
	override void OnChanges()
	{
		if (_inputOption != m_BoundOption)
			Refresh();
	}

	void Refresh()
	{
		m_BoundOption = _inputOption;
		if (_inputOption)
			option_label = _inputOption.label;
		MarkForCheck();
	}

	void OnOptionClick()
	{
		if (_inputOption && _inputOption.owner)
			_inputOption.owner.ApplyPresetOption(_inputOption);
	}
}
