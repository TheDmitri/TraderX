class CategoryCardView: ScriptViewTemplate<CategoryCardViewController>
{
	void CategoryCardView(TraderXCategory category, int itemCardViewSize, bool expand = false, int categoryCardType = 0)
	{
		GetTemplateController().SetCategoryCardData(category, itemCardViewSize, expand, categoryCardType);
	}

    override string GetLayoutFile() 
	{
		return string.Empty;
	}
}

class CategoryCardLargeView: CategoryCardView
{
	void CategoryCardLargeView(TraderXCategory category, int itemCardViewSize, bool expand = false, int categoryCardType = 0)
	{
		GetTemplateController().SetCategoryCardData(category, itemCardViewSize, expand, categoryCardType);
	}

	override string GetLayoutFile() 
	{
		return "TraderX/datasets/gui/TraderXPrefab/CategoryCard/CategoryCardLargeView.layout";
	}
}

class CategoryCardMediumView: CategoryCardView
{
	void CategoryCardMediumView(TraderXCategory category, int itemCardViewSize, bool expand = false, int categoryCardType = 0)
	{
		GetTemplateController().SetCategoryCardData(category, itemCardViewSize, expand, categoryCardType);
	}

	override string GetLayoutFile() 
	{
		return "TraderX/datasets/gui/TraderXPrefab/CategoryCard/CategoryCardMediumView.layout";
	}
}