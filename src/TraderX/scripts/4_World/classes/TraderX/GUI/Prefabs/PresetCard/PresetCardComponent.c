class PresetCardComponent extends DZlrComponentBase
{
    private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("traderx-preset-card", PresetCardComponent);

    static ref ScriptInvoker Event_OnPresetClickCallBack = new ScriptInvoker();
    static ref ScriptInvoker Event_OnDefaultPresetClickCallBack = new ScriptInvoker();
    static ref ScriptInvoker Event_OnEditPresetClickCallBack = new ScriptInvoker();
    static ref ScriptInvoker Event_OnDeletePresetClickCallBack = new ScriptInvoker();

    ref TraderXPreset preset;
    bool isServerPreset;
    EntityAI preview;
    string price, preset_name;

    ButtonWidget defaultPresetBtn, editBtn, deleteBtn, presetBtn;
    ImageWidget defaultPresetImg;
    TextWidget presetNameText, priceText;
    ItemPreviewWidget presetPreview;

    void PresetCardComponent(TraderXPreset preset, bool isServerPreset)
    {
        this.preset = preset;
        this.isServerPreset = isServerPreset;
    }

    override protected void RegisterMetadata()
    {
        m_Metadata = new ComponentMetadata();
        m_Metadata.selector = "traderx-preset-card";
        m_Metadata.templatePath = "TraderX/datasets/gui/TraderXPrefab/Preset/PresetCardView.layout";
    }

    override void OnInit()
    {
        if (isServerPreset)
            preset_name = "[SERVER] " + preset.presetName;
        else
            preset_name = preset.presetName;

        int intPrice = TraderXPresetsService.GetInstance().CalculateTotalPricePreset(preset);
        price = TraderXQuantityManager.GetFormattedMoneyAmount(intPrice);

        TraderXProduct item = TraderXProductRepository.GetItemById(preset.productId);
        if (item)
            preview = EntityAI.Cast(GetGame().CreateObjectEx(item.className, vector.Zero, ECE_LOCAL | ECE_NOLIFETIME));

        if (isServerPreset)
        {
            if (editBtn) editBtn.Show(false);
            if (deleteBtn) deleteBtn.Show(false);
        }

        if (presetNameText) presetNameText.SetText(preset_name);
        if (priceText) priceText.SetText(price);
        if (presetPreview) presetPreview.SetItem(preview);
    }

    override void OnDestroy()
    {
        if (preview) { GetGame().ObjectDelete(preview); preview = null; }
    }

    void RefreshDefaultStatus(string productId)
    {
        // Refresh default button color based on current default preset
        TraderXPreset defaultPreset = TraderXPresetsService.GetInstance().GetDefaultPreset(productId);
        bool isDefault = (defaultPreset && defaultPreset.presetId == preset.presetId);
        if (defaultPresetImg) defaultPresetImg.SetColor(TernaryInt(isDefault, TraderXViewStyles.TEXT_GREEN, 0xffffffff));
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (w == presetBtn) { Event_OnPresetClickCallBack.Invoke(preset); return true; }
        if (w == defaultPresetBtn) { Event_OnDefaultPresetClickCallBack.Invoke(preset); return true; }
        if (w == editBtn) { Event_OnEditPresetClickCallBack.Invoke(preset); return true; }
        if (w == deleteBtn) { Event_OnDeletePresetClickCallBack.Invoke(preset); return true; }
        return false;
    }

    override bool OnMouseEnter(Widget w, int x, int y)
    {
        if (w && w.GetUserID() == 333) w.SetAlpha(1.0);
        return false;
    }

    override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
    {
        if (w && w.GetUserID() == 333) w.SetAlpha(0.70);
        return false;
    }
}
