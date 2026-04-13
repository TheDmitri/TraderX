class NewPresetCardComponent extends DZlrComponentBase
{
    private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("traderx-new-preset-card", NewPresetCardComponent);
    static ref ScriptInvoker Event_OnNewPresetClickCallBack = new ScriptInvoker();

    ButtonWidget newPresetBtn;

    override protected void RegisterMetadata()
    {
        m_Metadata = new ComponentMetadata();
        m_Metadata.selector = "traderx-new-preset-card";
        m_Metadata.templatePath = "TraderX/datasets/gui/TraderXPrefab/Preset/NewPresetCardView.layout";
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (w == newPresetBtn) { Event_OnNewPresetClickCallBack.Invoke(); return true; }
        return false;
    }
}
