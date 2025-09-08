class CustomizeTooltipView: ScriptViewTemplate<CustomizeTooltipViewController>
{
    void CustomizeTooltipView(string title)
    {
        m_TemplateController.Setup(title);

        Show(true);

        SetPositionTooltip();
    }

    override string GetLayoutFile() 
    {
        return "TraderX/datasets/gui/TraderXPrefab/Tooltip/CustomizeTooltip.layout";
    }

    void SetPositionTooltip()
	{
		int x, y;
		GetMousePos(x, y);
		int w, h;
		GetScreenSize(w, h);
		GetLayoutRoot().SetPos(x + 10.0, y - 10.0, true);
		GetLayoutRoot().Update();
	}
}