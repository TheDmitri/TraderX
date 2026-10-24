class PresetCardComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("tx-preset-card", PresetCardComponent);

	static ref ScriptInvoker Event_OnPresetClickCallBack = new ScriptInvoker();
	static ref ScriptInvoker Event_OnDefaultPresetClickCallBack = new ScriptInvoker();
	static ref ScriptInvoker Event_OnEditPresetClickCallBack = new ScriptInvoker();
	static ref ScriptInvoker Event_OnDeletePresetClickCallBack = new ScriptInvoker();

	ref TraderXPreset _inputPreset;
	bool isServerPreset;

	EntityAI preview;
	string price;
	string preset_name;
	// \"server default\" / \"yours\" (Nasdara preset row)
	string preset_owner;

	ButtonWidget defaultPresetBtn;
	ButtonWidget editBtn;
	ButtonWidget deleteBtn;
	ButtonWidget presetBtn;
	ImageWidget defaultPresetImg;

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "tx-preset-card";
		m_Metadata.templatePath = "TraderX/datasets/gui/TraderXPrefab/Preset/PresetCardView.layout";
	}

	override void OnInit()
	{
		AutoLoadWidgets();
		if (_inputPreset)
			Setup(_inputPreset, false);
	}

	override void OnDestroy()
	{
		if (preview)
		{
			GetGame().ObjectDelete(preview);
			preview = null;
		}
	}

	void Setup(TraderXPreset preset, bool serverPreset)
	{
		_inputPreset = preset;
		isServerPreset = serverPreset;

		preset_name = preset.presetName;
		if (isServerPreset)
			preset_owner = Widget.TranslateString("#tpm_preset_server_default");
		else
			preset_owner = Widget.TranslateString("#tpm_preset_yours");

		int intPrice = TraderXPresetsService.GetInstance().CalculateTotalPricePreset(preset);
		price = TraderXCurrencyService.GetInstance().GetTraderCurrencySymbol() + TraderXQuantityManager.GetFormattedMoneyAmount(intPrice);

		TraderXProduct item = TraderXProductRepository.GetItemById(preset.productId);
		preview = EntityAI.Cast(GetGame().CreateObjectEx(item.className, vector.Zero, ECE_LOCAL|ECE_NOLIFETIME));
		CreatePresetPreviewWithAttachments();

		if (preset)
			UpdateDefaultButtonColor();

		ConfigureServerPresetUI();
		MarkForCheck();
	}

	void CreatePresetPreviewWithAttachments()
	{
		if (!preview || !_inputPreset) return;
		foreach (string attachmentId : _inputPreset.attachments)
		{
			TraderXProduct attachmentProduct = TraderXProductRepository.GetItemById(attachmentId);
			if (!attachmentProduct) continue;
			EntityAI attachmentEntity = EntityAI.Cast(GetGame().CreateObjectEx(attachmentProduct.className, vector.Zero, ECE_LOCAL|ECE_NOLIFETIME));
			if (!attachmentEntity) continue;
			bool attached = TraderXAttachmentHandler.TryAttachItem(preview, attachmentEntity);
			if (!attached)
				GetGame().ObjectDelete(attachmentEntity);
		}
	}

	void OnPresetClick()
	{
		Event_OnPresetClickCallBack.Invoke(_inputPreset);
	}

	void OnDefaultPresetClick()
	{
		Event_OnDefaultPresetClickCallBack.Invoke(_inputPreset);
	}

	void OnEditPresetClick()
	{
		Event_OnEditPresetClickCallBack.Invoke(_inputPreset);
	}

	void OnDeletePresetClick()
	{
		Event_OnDeletePresetClickCallBack.Invoke(_inputPreset);
	}

	void UpdateDefaultButtonColor()
	{
		UpdateDefaultButtonColorForProduct(_inputPreset.productId);
	}

	void UpdateDefaultButtonColorForProduct(string storageProductId)
	{
		if (!defaultPresetBtn || !defaultPresetImg || !_inputPreset) return;
		string defaultPresetId = TraderXPresetsService.GetInstance().GetDefaultPresetId(storageProductId);
		bool isDefault = (defaultPresetId == _inputPreset.presetId);
		if (isDefault)
			defaultPresetImg.SetColor(TraderXTheme.PRESET_DEFAULT);
		else
			defaultPresetImg.SetColor(TraderXTheme.PRESET_NORMAL);
	}

	void RefreshDefaultStatus(string storageProductId)
	{
		UpdateDefaultButtonColorForProduct(storageProductId);
	}

	void ConfigureServerPresetUI()
	{
		if (!isServerPreset) return;
		if (editBtn)
		{
			editBtn.Enable(false);
			editBtn.SetAlpha(0);
		}
		if (deleteBtn)
		{
			deleteBtn.Enable(false);
			deleteBtn.SetAlpha(0.5);
		}
		if (defaultPresetBtn)
		{
			defaultPresetBtn.Enable(false);
			defaultPresetBtn.SetAlpha(0.5);
		}
	}
}
