class BuyTooltipView: ScriptViewTemplate<BuyTooltipViewController>
{
    float offsetX;
    float offsetY;

    void BuyTooltipView(string title, bool hasVariants, bool hasAttachments, float offsetX, float offsetY)
    {
        this.offsetX = offsetX;
        this.offsetY = offsetY;

        m_TemplateController.Setup(title, hasVariants, hasAttachments);

        Show(true);

        SetPositionTooltip();
    }

    override string GetLayoutFile() 
	{
		return "TraderX/datasets/gui/TraderXPrefab/Tooltip/BuyTooltip.layout";
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