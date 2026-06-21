class NewPresetCardComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("tx-new-preset-card", NewPresetCardComponent);

	static ref ScriptInvoker Event_OnNewPresetClickCallBack = new ScriptInvoker();

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "tx-new-preset-card";
		m_Metadata.templatePath = "TraderX/datasets/gui/TraderXPrefab/Preset/NewPresetCardView.layout";
	}

	void OnNewPresetClick()
	{
		Event_OnNewPresetClickCallBack.Invoke();
	}
}
