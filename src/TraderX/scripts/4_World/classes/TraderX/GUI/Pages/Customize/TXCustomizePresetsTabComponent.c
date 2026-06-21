// Customize presets tab. Renders server + user presets as tx-preset-card via
// Component_ForEach, plus a tx-new-preset-card. All preset CRUD lives here; the
// page shell (CustomizePageComponent) only provides the modal overlay surface.
class TXCustomizePresetsTabComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("tx-customize-presets", TXCustomizePresetsTabComponent);

	// The tab subscribes to STATIC modal/card events, but the {{ current_tab }} host can
	// leave previous tab instances mounted (and still subscribed) when you switch tabs.
	// One Confirm then fired every stale instance — creating N presets ("3 tactical setup")
	// while the visible tab never refreshed. Only the latest-mounted (visible) instance is
	// allowed to act on those events.
	private static TXCustomizePresetsTabComponent s_Active;

	ref array<ref TraderXPreset> _inputPresets = new array<ref TraderXPreset>();
	ref TraderXProduct originalItem;
	ref TraderXPreset clickedActionPreset;
	ref TraderXPreset editingPreset;   // set while the create modal is reused for rename
	string empty_label;

	bool IsActive()
	{
		return s_Active == this;
	}

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "tx-customize-presets";
		m_Metadata.templatePath = "TraderX/datasets/gui/PurchasePage/Customize/CustomizePresetsTabView.layout";
	}

	override void OnInit()
	{
		s_Active = this;
		originalItem = CustomizeStateService.GetInstance().GetOriginalItem();

		PresetCardComponent.Event_OnPresetClickCallBack.Insert(OnPresetClick);
		PresetCardComponent.Event_OnDefaultPresetClickCallBack.Insert(OnDefaultPresetClick);
		PresetCardComponent.Event_OnEditPresetClickCallBack.Insert(OnEditPresetClick);
		PresetCardComponent.Event_OnDeletePresetClickCallBack.Insert(OnDeletePresetClick);
		NewPresetCardComponent.Event_OnNewPresetClickCallBack.Insert(OnNewPresetClick);
		ModalCreatePresetComponent.Event_OnConfirmClickCallBack.Insert(OnConfirmCreate);
		ModalCreatePresetComponent.Event_OnCancelClickCallBack.Insert(OnCancelModal);
		ModalConfirmationComponent.Event_OnConfirmClickCallBack.Insert(OnConfirmDelete);
		ModalConfirmationComponent.Event_OnCancelClickCallBack.Insert(OnCancelModal);

		FillPresetList();
	}

	override void OnDestroy()
	{
		if (s_Active == this)
			s_Active = null;

		PresetCardComponent.Event_OnPresetClickCallBack.Remove(OnPresetClick);
		PresetCardComponent.Event_OnDefaultPresetClickCallBack.Remove(OnDefaultPresetClick);
		PresetCardComponent.Event_OnEditPresetClickCallBack.Remove(OnEditPresetClick);
		PresetCardComponent.Event_OnDeletePresetClickCallBack.Remove(OnDeletePresetClick);
		NewPresetCardComponent.Event_OnNewPresetClickCallBack.Remove(OnNewPresetClick);
		ModalCreatePresetComponent.Event_OnConfirmClickCallBack.Remove(OnConfirmCreate);
		ModalCreatePresetComponent.Event_OnCancelClickCallBack.Remove(OnCancelModal);
		ModalConfirmationComponent.Event_OnConfirmClickCallBack.Remove(OnConfirmDelete);
		ModalConfirmationComponent.Event_OnCancelClickCallBack.Remove(OnCancelModal);
	}

	void FillPresetList()
	{
		if (!originalItem) return;

		_inputPresets.Clear();

		// Server presets (admin-configured) first
		TraderXPresets serverPresets = TraderXPresetsService.GetInstance().GetServerPresets(originalItem.productId);
		if (serverPresets && serverPresets.presets)
		{
			foreach (TraderXPreset serverPreset : serverPresets.presets)
				_inputPresets.Insert(serverPreset);
		}

		// User presets
		array<ref TraderXPreset> userPresets = TraderXPresetsService.GetInstance().GetPresets(originalItem.productId);
		foreach (TraderXPreset userPreset : userPresets)
			_inputPresets.Insert(userPreset);

		empty_label = TernaryString(_inputPresets.Count() == 0, "#tpm_no_presets", "");
		MarkForCheck();
	}

	void OnPresetClick(TraderXPreset preset)
	{
		if (!IsActive()) return;
		TraderXSelectionService.GetInstance().DeSelectAllItems();
		TraderXSelectionService.GetInstance().SelectItem(originalItem);

		map<string, int> attachmentCounts = new map<string, int>();
		foreach (string attachmentId : preset.attachments)
		{
			if (attachmentCounts.Contains(attachmentId))
				attachmentCounts.Set(attachmentId, attachmentCounts.Get(attachmentId) + 1);
			else
				attachmentCounts.Set(attachmentId, 1);
		}
		foreach (string attId, int count : attachmentCounts)
		{
			TraderXProduct att = TraderXProductRepository.GetItemById(attId);
			if (att)
			{
				att.SetMultiplier(count);
				TraderXSelectionService.GetInstance().SelectItem(att);
			}
		}
		originalItem.defaultPreset = preset;
		CustomizeStateService.GetInstance().SetCurrentPreset(preset);
	}

	void OnDefaultPresetClick(TraderXPreset preset)
	{
		if (!IsActive()) return;
		if (!preset || !originalItem) return;
		if (IsServerPreset(preset)) return;
		TraderXPresetsService.GetInstance().SetDefaultPreset(originalItem.productId, preset.presetId);
		FillPresetList();
	}

	void OnEditPresetClick(TraderXPreset preset)
	{
		if (!IsActive()) return;
		if (!preset || IsServerPreset(preset)) return;

		// Reuse the create modal as a rename dialog: prefill the current name + retitle.
		editingPreset = preset;
		ModalCreatePresetComponent.s_PendingName = preset.presetName;
		ModalCreatePresetComponent.s_PendingTitle = "#tpm_edit_preset_name";
		CustomizePageComponent.GetInstance().ShowModal("tx-modal-preset");
	}

	void OnDeletePresetClick(TraderXPreset preset)
	{
		if (!IsActive()) return;
		if (IsServerPreset(preset)) return;
		clickedActionPreset = preset;
		CustomizePageComponent.GetInstance().ShowConfirmModal("Delete preset", "Are you sure you want to delete this preset?");
	}

	void OnNewPresetClick()
	{
		if (!IsActive()) return;
		// Create mode: empty name + create title.
		editingPreset = null;
		ModalCreatePresetComponent.s_PendingName = "";
		ModalCreatePresetComponent.s_PendingTitle = "#tpm_create_new_preset";
		CustomizePageComponent.GetInstance().ShowModal("tx-modal-preset");
	}

	// One handler for both modes (the modal can't tell create from rename).
	void OnConfirmCreate(string name)
	{
		if (!IsActive()) return;

		if (editingPreset)
		{
			TraderXPresetsService.GetInstance().RenamePreset(originalItem.productId, editingPreset.presetId, name);
			editingPreset = null;
			CustomizePageComponent.GetInstance().HideModal();
			FillPresetList();
			GetTraderXToastService().ShowToast(TXNOTIF_INFO, "#tpm_preset_renamed", name);
			return;
		}

		bool success = CustomizeStateService.GetInstance().CreatePreset(name);
		CustomizePageComponent.GetInstance().HideModal();
		if (success)
		{
			FillPresetList();
			GetTraderXToastService().ShowToast(TXNOTIF_INFO, "#tpm_preset_created", name);
		}
	}

	void OnConfirmDelete()
	{
		if (!IsActive()) return;
		CustomizePageComponent.GetInstance().HideModal();
		if (!clickedActionPreset) return;
		TraderXPresetsService.GetInstance().RemovePreset(originalItem.productId, clickedActionPreset.presetId);
		clickedActionPreset = null;
		FillPresetList();
	}

	void OnCancelModal()
	{
		editingPreset = null;
		CustomizePageComponent.GetInstance().HideModal();
	}

	bool IsServerPreset(TraderXPreset preset)
	{
		if (!preset || !originalItem) return false;
		TraderXPresets serverPresets = TraderXPresetsService.GetInstance().GetServerPresets(originalItem.productId);
		if (!serverPresets || !serverPresets.presets) return false;
		foreach (TraderXPreset sp : serverPresets.presets)
		{
			if (sp.presetId == preset.presetId) return true;
		}
		return false;
	}
}
