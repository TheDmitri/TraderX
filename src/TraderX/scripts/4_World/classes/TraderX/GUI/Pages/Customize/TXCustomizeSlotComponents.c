// Customize attachments tab (Nasdara): one row per attachment slot of the product
// (HANDGUARD, BUTTSTOCK, MAGAZINE...) with the part fitted in it or "Empty", its price and
// a Change button that unfolds the slot's options; and the preset chips below.

// One attachment slot and the products that fit it.
class TXCustomizeSlot
{
	string slotName;
	string label;
	ref array<ref TraderXProduct> options = new array<ref TraderXProduct>();

	// Inventory slot of an attachment product: the first inventorySlot[] of its config
	// class, "magazine" for magazines.
	static string SlotOf(string className)
	{
		TStringArray roots = {"CfgVehicles", "CfgWeapons", "CfgMagazines"};
		foreach (string root : roots)
		{
			string path = root + " " + className;
			if (!GetGame().ConfigIsExisting(path)) continue;
			if (root == "CfgMagazines") return "magazine";
			TStringArray slots = new TStringArray();
			GetGame().ConfigGetTextArray(path + " inventorySlot", slots);
			if (slots.Count() > 0) return slots[0];
			return "";
		}
		return "";
	}

	// "HANDGUARD": the game's slot name (CfgSlots displayName), uppercase.
	// (locals must not share a member name: Enforce resolves them to the member)
	static string LabelOf(string slotId)
	{
		string text = "";
		GetGame().ConfigGetText("CfgSlots Slot_" + slotId + " displayName", text);
		if (text == "") text = slotId;
		text = Widget.TranslateString(text);
		text.ToUpper();
		return text;
	}

	// Group a product's attachments by slot, in order of first appearance.
	static void Build(TraderXProduct item, array<ref TXCustomizeSlot> outSlots)
	{
		outSlots.Clear();
		if (!item) return;
		map<string, ref TXCustomizeSlot> bySlot = new map<string, ref TXCustomizeSlot>();
		// keep the returned array alive while iterating (1.30 frees temporaries)
		array<ref TraderXProduct> attachments = item.GetAttachments();
		foreach (TraderXProduct attachment : attachments)
		{
			if (!attachment) continue;
			string slotId = SlotOf(attachment.className);
			string key = slotId;
			key.ToLower();
			if (key == "") key = attachment.className;
			TXCustomizeSlot slot;
			if (!bySlot.Find(key, slot))
			{
				slot = new TXCustomizeSlot();
				slot.slotName = slotId;
				slot.label = LabelOf(slotId);
				bySlot.Insert(key, slot);
				outSlots.Insert(slot);
			}
			slot.options.Insert(attachment);
		}
	}

	TraderXProduct GetFitted()
	{
		foreach (TraderXProduct option : options)
		{
			if (CustomizePreviewService.GetInstance().HasAttachment(option))
				return option;
		}
		return null;
	}

	bool Contains(TraderXProduct product)
	{
		if (!product) return false;
		foreach (TraderXProduct option : options)
		{
			if (option.productId == product.productId) return true;
		}
		return false;
	}

	// Fit `product` in this slot (taking out the current part), or empty the slot when the
	// product is already fitted. The cart follows the selection.
	void Toggle(TraderXProduct product)
	{
		TraderXProduct fitted = GetFitted();
		if (fitted)
		{
			CustomizePreviewService.GetInstance().RemoveAttachment(fitted);
			array<string> ids = new array<string>();
			ids.Insert(fitted.productId);
			TraderXSelectionService.GetInstance().DeselectItems(ids);
		}
		if (fitted && fitted.productId == product.productId)
			return;
		TraderXSelectionService.GetInstance().SelectItem(product);
		CustomizePreviewService.GetInstance().AddAttachment(product);
	}
}

class TXCustomizeSlotRowComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("tx-customize-slot-row", TXCustomizeSlotRowComponent);

	ref TXCustomizeSlot _inputSlot;
	ref array<ref TraderXProduct> _inputOptions = new array<ref TraderXProduct>();
	string slot_label;
	string fitted_name;
	string fitted_price;
	// local copy of the fitted part, shown in the row's thumbnail
	EntityAI fitted_preview;
	bool m_Open;

	TextWidget fittedName;
	TextWidget emptyName;
	Widget slotOptionsGrid;
	Widget slotFittedPreview;

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "tx-customize-slot-row";
		m_Metadata.templatePath = "TraderX/datasets/gui/PurchasePage/Customize/CustomizeSlotRowView.layout";
	}

	override void OnInit()
	{
		AutoLoadWidgets();
		CustomizePreviewService.Event_OnAttachmentsChanged.Insert(OnAttachmentsChanged);
		TXCustomizeSlotOptionComponent.Event_OnOptionClick.Insert(OnOptionClick);
		Refresh();
	}

	override void OnDestroy()
	{
		CustomizePreviewService.Event_OnAttachmentsChanged.Remove(OnAttachmentsChanged);
		TXCustomizeSlotOptionComponent.Event_OnOptionClick.Remove(OnOptionClick);
		SetFittedPreview("");
	}

	void SetFittedPreview(string className)
	{
		if (fitted_preview && fitted_preview.GetType() == className)
			return;
		if (fitted_preview)
			GetGame().ObjectDelete(fitted_preview);
		fitted_preview = null;
		if (className != "")
			fitted_preview = EntityAI.Cast(GetGame().CreateObjectEx(className, vector.Zero, ECE_LOCAL|ECE_NOLIFETIME));
	}

	override void OnChanges()
	{
		Refresh();
	}

	void Refresh()
	{
		if (!_inputSlot) return;
		slot_label = _inputSlot.label;
		TraderXProduct fitted = _inputSlot.GetFitted();
		if (fitted)
		{
			fitted_name = fitted.GetDisplayName();
			int price = TraderXPricingService.GetInstance().GetPricePreview(fitted.GetProductId(), true, 1, TraderXItemState.PRISTINE);
			fitted_price = TraderXCurrencyService.GetInstance().GetTraderCurrencySymbol() + TraderXQuantityManager.GetFormattedMoneyAmount(price);
			SetFittedPreview(fitted.className);
		}
		else
		{
			fitted_name = "";
			fitted_price = "";
			SetFittedPreview("");
		}
		if (fittedName) fittedName.Show(fitted != null);
		if (emptyName) emptyName.Show(fitted == null);
		if (slotOptionsGrid) slotOptionsGrid.Show(m_Open);
		MarkForCheck();
	}

	void OnChangeClick()
	{
		TraderXInventoryManager.PlayMenuSound(ETraderXSounds.SELECT);
		m_Open = !m_Open;
		_inputOptions.Clear();
		if (m_Open)
		{
			foreach (TraderXProduct option : _inputSlot.options)
				_inputOptions.Insert(option);
		}
		Refresh();
	}

	void OnOptionClick(TraderXProduct product)
	{
		if (!_inputSlot || !_inputSlot.Contains(product)) return;
		TraderXInventoryManager.PlayMenuSound(ETraderXSounds.SELECT);
		_inputSlot.Toggle(product);
		m_Open = false;
		_inputOptions.Clear();
		Refresh();
	}

	void OnAttachmentsChanged(array<ref TraderXProduct> attachments)
	{
		Refresh();
		TXCustomizeSlotOptionComponent.Event_OnOptionsRefresh.Invoke();
	}
}

class TXCustomizeSlotOptionComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("tx-customize-slot-option", TXCustomizeSlotOptionComponent);

	// (TraderXProduct) an option was clicked; the row owning it fits it
	static ref ScriptInvoker Event_OnOptionClick = new ScriptInvoker();
	static ref ScriptInvoker Event_OnOptionsRefresh = new ScriptInvoker();

	ref TraderXProduct _inputProduct;
	string option_name;
	string option_price;
	Widget optionFittedIcon;

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "tx-customize-slot-option";
		m_Metadata.templatePath = "TraderX/datasets/gui/PurchasePage/Customize/CustomizeSlotOptionView.layout";
	}

	override void OnInit()
	{
		AutoLoadWidgets();
		Event_OnOptionsRefresh.Insert(Refresh);
		Refresh();
	}

	override void OnDestroy()
	{
		Event_OnOptionsRefresh.Remove(Refresh);
	}

	override void OnChanges()
	{
		Refresh();
	}

	void Refresh()
	{
		if (!_inputProduct) return;
		option_name = _inputProduct.GetDisplayName();
		int price = TraderXPricingService.GetInstance().GetPricePreview(_inputProduct.GetProductId(), true, 1, TraderXItemState.PRISTINE);
		option_price = TraderXCurrencyService.GetInstance().GetTraderCurrencySymbol() + TraderXQuantityManager.GetFormattedMoneyAmount(price);
		if (optionFittedIcon) optionFittedIcon.Show(CustomizePreviewService.GetInstance().HasAttachment(_inputProduct));
		MarkForCheck();
	}

	void OnOptionClick()
	{
		Event_OnOptionClick.Invoke(_inputProduct);
	}
}

// "YOUR PRESETS" chip: name + "€ 670 · server default" / "· yours"; the active preset is
// outlined in ember.
class TXCustomizePresetChipComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("tx-customize-preset-chip", TXCustomizePresetChipComponent);

	ref TraderXPreset _inputPreset;
	string chip_name;
	string chip_detail;
	Widget chipActiveFrame;

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "tx-customize-preset-chip";
		m_Metadata.templatePath = "TraderX/datasets/gui/PurchasePage/Customize/CustomizePresetChipView.layout";
	}

	override void OnInit()
	{
		AutoLoadWidgets();
		CustomizeStateService.Event_OnPresetChanged.Insert(OnPresetChanged);
		Refresh();
	}

	override void OnDestroy()
	{
		CustomizeStateService.Event_OnPresetChanged.Remove(OnPresetChanged);
	}

	override void OnChanges()
	{
		Refresh();
	}

	void OnPresetChanged(TraderXPreset preset)
	{
		Refresh();
	}

	void Refresh()
	{
		if (!_inputPreset) return;
		chip_name = _inputPreset.presetName;
		int total = TraderXPresetsService.GetInstance().CalculateTotalPricePreset(_inputPreset);
		string owner = Widget.TranslateString("#tpm_preset_yours");
		TraderXProduct item = CustomizeStateService.GetInstance().GetOriginalItem();
		if (item)
		{
			TraderXPresets serverPresets = TraderXPresetsService.GetInstance().GetServerPresets(item.productId);
			if (serverPresets && serverPresets.presets)
			{
				foreach (TraderXPreset sp : serverPresets.presets)
				{
					if (sp.presetId == _inputPreset.presetId)
						owner = Widget.TranslateString("#tpm_preset_server_default");
				}
			}
		}
		chip_detail = TraderXCurrencyService.GetInstance().GetTraderCurrencySymbol() + TraderXQuantityManager.GetFormattedMoneyAmount(total) + " · " + owner;
		TraderXPreset current = CustomizeStateService.GetInstance().GetCurrentPreset();
		if (chipActiveFrame) chipActiveFrame.Show(current && current.presetId == _inputPreset.presetId);
		MarkForCheck();
	}

	void OnChipClick()
	{
		TraderXInventoryManager.PlayMenuSound(ETraderXSounds.SELECT);
		TXCustomizePresetsTabComponent.ApplyPreset(CustomizeStateService.GetInstance().GetOriginalItem(), _inputPreset);
	}
}
