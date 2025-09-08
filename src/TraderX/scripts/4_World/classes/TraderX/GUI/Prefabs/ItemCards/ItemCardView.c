
class ItemCardView: ScriptViewTemplate<ItemCardViewController>
{
    void ItemCardView(TraderXProduct item, int categoryType = ETraderXCategoryType.NONE, bool selectable = true, bool isFavable = true, bool isFav = false)
	{
		m_TemplateController.Setup(item, categoryType, selectable, isFavable, isFav);
	}

	static ItemCardView CreateItemCardView(TraderXProduct item, int size, int categoryType = ETraderXCategoryType.NONE, bool selectable = true, bool isFavable = true, bool isFav = false)
	{
		switch(size)
		{
			case EItemCardSize.SMALL:
				return new ItemCardSmallView(item, categoryType, selectable, isFavable, isFav);
			break;
			case EItemCardSize.MEDIUM:
				return new ItemCardMediumView(item, categoryType, selectable, isFavable, isFav);
			break;
			case EItemCardSize.LARGE:
				return new ItemCardLargeView(item, categoryType, selectable, isFavable, isFav);
			break;

			case EItemCardSize.TOOLTIP:
				return new ItemCardTooltipView(item, categoryType, selectable, isFavable, isFav);
			break;

			default: //large
				return new ItemCardLargeView(item, categoryType, selectable, isFavable, isFav);
			break;
		}

		return new ItemCardView(item, selectable, categoryType, isFavable, isFav);
	}

	override string GetLayoutFile() 
	{
		return "TraderX/datasets/gui/TraderXPrefab/ItemCard/ItemCardLargeView.layout";
	}

	override void Show(bool show)
	{
		m_TemplateController.OnShow(show);
		super.Show(show);
	}
}

class ItemCardSmallView : ItemCardView
{
	void ItemCardSmallView(TraderXProduct item, int categoryType = ETraderXCategoryType.NONE, bool selectable = true, bool isFavable = true, bool isFav = false)
    {
        m_TemplateController.Setup(item, categoryType, selectable, isFavable, isFav);
    }

    override string GetLayoutFile() 
    {
        return "TraderX/datasets/gui/TraderXPrefab/ItemCard/ItemCardSmallView.layout";
    }
}

class ItemCardTooltipView : ItemCardView
 {
    void ItemCardTooltipView(TraderXProduct item, int categoryType = ETraderXCategoryType.NONE,  bool selectable = true, bool isFavable = true, bool isFav = false)
    {
		m_TemplateController.Setup(item, categoryType, selectable, isFavable, isFav);
	}

	override string GetLayoutFile() 
	{
		return "TraderX/datasets/gui/TraderXPrefab/ItemCard/ItemCardMediumViewTooltip.layout";
	}
 }

class ItemCardMediumView : ItemCardView
 {
    void ItemCardMediumView(TraderXProduct item, int categoryType = ETraderXCategoryType.NONE,  bool selectable = true, bool isFavable = true, bool isFav = false)
    {
		m_TemplateController.Setup(item, categoryType, selectable, isFavable, isFav);
	}

	override string GetLayoutFile() 
	{
		return "TraderX/datasets/gui/TraderXPrefab/ItemCard/ItemCardMediumView.layout";
	}
 }

 class ItemCardLargeView : ItemCardView
 {
	void ItemCardLargeView(TraderXProduct item, int categoryType = ETraderXCategoryType.NONE, bool selectable = true, bool isFavable = true, bool isFav = false)
	{
		m_TemplateController.Setup(item, categoryType, selectable, isFavable, isFav);
	}

	override string GetLayoutFile() 
	{
		return "TraderX/datasets/gui/TraderXPrefab/ItemCard/ItemCardLargeView.layout";
	}
 }
