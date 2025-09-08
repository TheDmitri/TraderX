class PlayerSlotNavigationButtonView: ScriptViewTemplate<PlayerSlotNavigationButtonViewController>
{
    void PlayerSlotNavigationButtonView(int navBtnId, int color = -1)
    {
        m_TemplateController.Setup(navBtnId, color);
    }
    
    override string GetLayoutFile() 
	{
		return "TraderX/datasets/gui/TraderXPrefab/Navigation/PlayerSlotNavigationCard.layout";
	}
}