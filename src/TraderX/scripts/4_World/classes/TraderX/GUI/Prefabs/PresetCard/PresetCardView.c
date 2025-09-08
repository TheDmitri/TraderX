class PresetCardView: ScriptViewTemplate<PresetCardViewController>
{
	void PresetCardView(TraderXPreset preset, bool isServerPreset){
		m_TemplateController.Setup(preset, isServerPreset);
	}

    override string GetLayoutFile() 
	{
		return "TraderX/datasets/gui/TraderXPrefab/Preset/PresetCardView.layout";
	}

    override bool OnMouseEnter(Widget w, int x, int y)
	{
        if(w && w.GetUserID() == 333){
            w.SetAlpha(1.0);
        }

        return false;
	}

    override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
        if(w && w.GetUserID() == 333){
            w.SetAlpha(0.70);
        }
            
        return false;
	}
}