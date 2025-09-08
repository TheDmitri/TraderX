class PresetCardViewController: ViewController
{
    EntityAI preview;
    string price, preset_name;

    static ref ScriptInvoker Event_OnPresetClickCallBack = new ScriptInvoker();
    static ref ScriptInvoker Event_OnDefaultPresetClickCallBack = new ScriptInvoker();
    static ref ScriptInvoker Event_OnEditPresetClickCallBack = new ScriptInvoker();
    static ref ScriptInvoker Event_OnDeletePresetClickCallBack = new ScriptInvoker();

    ref TraderXPreset preset;
    bool isServerPreset;

    ButtonWidget defaultPresetBtn, editBtn, deleteBtn, presetBtn;

    ImageWidget defaultPresetImg;

    void ~PresetCardViewController()
    {
        // Clean up preview entity when controller is destroyed
        if (preview)
        {
            GetGame().ObjectDelete(preview);
            preview = null;
        }
    }

    void Setup(TraderXPreset preset, bool isServerPreset)
    {
        this.preset = preset;
        this.isServerPreset = isServerPreset;
        
        // Add server indicator to preset name
        if (isServerPreset)
        {
            preset_name = "[SERVER] " + preset.presetName;
        }
        else
        {
            preset_name = preset.presetName;
        }
        
        int intPrice = TraderXPresetsService.GetInstance().CalculateTotalPricePreset(preset);
        price = TraderXQuantityManager.GetFormattedMoneyAmount(intPrice);

        TraderXProduct item = TraderXProductRepository.GetItemById(preset.productId);

        // Create base item preview
        preview = EntityAI.Cast(GetGame().CreateObjectEx(item.className, vector.Zero, ECE_LOCAL|ECE_NOLIFETIME));
        
        // Add attachments to preview
        CreatePresetPreviewWithAttachments();
        
        if (preset)
        {
            UpdateDefaultButtonColor();
        }
        
        // Configure UI elements based on preset type
        ConfigureServerPresetUI();

        NotifyPropertiesChanged({"preset_name", "price", "preview"});
    }
    
    void CreatePresetPreviewWithAttachments()
    {
        if (!preview || !preset)
            return;
            
        // Attach each configured attachment to the preview
        foreach (string attachmentId : preset.attachments)
        {
            TraderXProduct attachmentProduct = TraderXProductRepository.GetItemById(attachmentId);
            if (!attachmentProduct)
            {
                GetTraderXLogger().LogWarning(string.Format("CreatePresetPreviewWithAttachments - Attachment product not found: %1", attachmentId));
                continue;
            }
            
            // Create attachment entity
            EntityAI attachmentEntity = EntityAI.Cast(GetGame().CreateObjectEx(attachmentProduct.className, vector.Zero, ECE_LOCAL|ECE_NOLIFETIME));
            if (!attachmentEntity)
            {
                GetTraderXLogger().LogWarning(string.Format("CreatePresetPreviewWithAttachments - Failed to create attachment entity: %1", attachmentProduct.className));
                continue;
            }
            
            // Try to attach to preview
            bool attached = TraderXAttachmentHandler.TryAttachItem(preview, attachmentEntity);
            if (attached)
            {
                GetTraderXLogger().LogDebug(string.Format("CreatePresetPreviewWithAttachments - Successfully attached: %1", attachmentProduct.GetDisplayName()));
            }
            else
            {
                GetTraderXLogger().LogWarning(string.Format("CreatePresetPreviewWithAttachments - Failed to attach: %1", attachmentProduct.GetDisplayName()));
                // Clean up failed attachment
                GetGame().ObjectDelete(attachmentEntity);
            }
        }
        
        GetTraderXLogger().LogDebug(string.Format("CreatePresetPreviewWithAttachments - Completed for preset: %1", preset.presetName));
    }

    override void OnWidgetScriptInit(Widget w)
    {
        super.OnWidgetScriptInit(w);
    }

    bool OnPresetClickExecute(ButtonCommandArgs args)
    {
        Event_OnPresetClickCallBack.Invoke(preset);
        return true;
    }

    override bool OnClick(Widget w, int x, int y, int button)
	{
        switch(w)
        {
            case defaultPresetBtn:
                Event_OnDefaultPresetClickCallBack.Invoke(preset);
                return true;
            break;

            case editBtn:
                Event_OnEditPresetClickCallBack.Invoke(preset);
                return true;
            break;

            case deleteBtn:
                Event_OnDeletePresetClickCallBack.Invoke(preset);
                return true;
            break;

            case presetBtn:
                Event_OnPresetClickCallBack.Invoke(preset);
                return true;
            break;
        }
        return false;
    }
    
    void UpdateDefaultButtonColor()
    {
        if (!defaultPresetBtn || !defaultPresetImg || !preset)
        {
            return;
        }
            
        // Check if this preset is the default preset
        UUID defaultPresetId = TraderXPresetsService.GetInstance().GetDefaultPresetId(preset.productId);
        bool isDefault = (defaultPresetId == preset.presetId);
        
        if (isDefault)
        {
            // Set red color for default preset
            defaultPresetImg.SetColor(ARGB(255, 255, 0, 0)); // Red color
        }
        else
        {
            // Reset to normal color
            defaultPresetImg.SetColor(ARGB(255, 255, 255, 255)); // White/normal color
        }
    }
    
    void RefreshDefaultStatus()
    {
        UpdateDefaultButtonColor();
    }
    
    void ConfigureServerPresetUI()
    {
        if (!isServerPreset)
            return;
            
        // Disable edit and delete buttons for server presets
        if (editBtn)
        {
            editBtn.Enable(false);
            editBtn.SetAlpha(0); // Make it visually disabled
        }
        
        if (deleteBtn)
        {
            deleteBtn.Enable(false);
            deleteBtn.SetAlpha(0.5); // Make it visually disabled
        }
        
        // Disable default preset button for server presets (users can't set server presets as their default)
        if (defaultPresetBtn)
        {
            defaultPresetBtn.Enable(false);
            defaultPresetBtn.SetAlpha(0.5); // Make it visually disabled
        }
    }
}