class SimpleBtnTooltipView: ScriptViewTemplate<SimpleBtnTooltipViewController>
{
    float offsetX;
    float offsetY;
    
    void SimpleBtnTooltipView(string title, float offsetX, float offsetY)
    {
        this.offsetX = offsetX;
        this.offsetY = offsetY;

        m_TemplateController.Setup(title);

        Show(true);

        SetPositionTooltip();
    }

    override string GetLayoutFile() 
	{
		return "TraderXCore/datasets/gui/Prefab/Tooltip/SimpleBtnTooltip.layout";
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