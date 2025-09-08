class FavoritePageView: ScriptViewTemplate<FavoritePageViewController>
{
    void FavoritePageView()
    {
        m_TemplateController.Setup();
    }

    override string GetLayoutFile() 
	{
		return "TraderX/datasets/gui/Favorites/FavoritesView.layout";
	}
}