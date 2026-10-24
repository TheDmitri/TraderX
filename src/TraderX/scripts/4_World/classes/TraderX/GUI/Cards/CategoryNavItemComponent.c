// Category entry of the purchase page sidebar (Nasdara): name + number of products the
// trader sells in it; the selected one is filled and bold. Clicks go to the page through
// Event_OnCategoryNavClick; the page announces its choice with SetSelected.
class CategoryNavItemComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("tx-category-nav-item", CategoryNavItemComponent);

	// (TraderXCategory) an entry was clicked
	static ref ScriptInvoker Event_OnCategoryNavClick = new ScriptInvoker();
	// (TraderXCategory) the page selected a category
	static ref ScriptInvoker Event_OnCategoryNavSelected = new ScriptInvoker();
	protected static TraderXCategory s_Selected;

	ref TraderXCategory _inputCategory;
	string nav_name;
	string nav_count;

	Widget navSelectedBg;
	TextWidget navName;
	TextWidget navNameBold;
	Widget navLockIcon;

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "tx-category-nav-item";
		m_Metadata.templatePath = "TraderX/datasets/gui/TraderXPrefab/CategoryCard/CategoryNavItemView.layout";
	}

	static void SetSelected(TraderXCategory category)
	{
		s_Selected = category;
		Event_OnCategoryNavSelected.Invoke(category);
	}

	override void OnInit()
	{
		AutoLoadWidgets();
		Event_OnCategoryNavSelected.Insert(OnSelected);
		TraderXLicenseService.Event_OnPlayerLicensesChanged.Insert(Refresh);
		Refresh();
	}

	override void OnDestroy()
	{
		Event_OnCategoryNavSelected.Remove(OnSelected);
		TraderXLicenseService.Event_OnPlayerLicensesChanged.Remove(Refresh);
	}

	override void OnChanges()
	{
		Refresh();
	}

	void Refresh()
	{
		if (!_inputCategory) return;
		nav_name = Widget.TranslateString(_inputCategory.categoryName);
		int count = 0;
		array<ref TraderXProduct> products = _inputCategory.GetProducts();
		if (products)
		{
			foreach (TraderXProduct item : products)
			{
				if (item && item.CanBeBought())
					count++;
			}
		}
		nav_count = count.ToString();
		if (navLockIcon) navLockIcon.Show(!TraderXLicenseService.GetInstance().HasLicenses(_inputCategory.licensesRequired));
		ApplySelected();
		MarkForCheck();
	}

	void ApplySelected()
	{
		bool selected = _inputCategory && _inputCategory == s_Selected;
		if (navSelectedBg) navSelectedBg.Show(selected);
		if (navName) navName.Show(!selected);
		if (navNameBold) navNameBold.Show(selected);
	}

	void OnSelected(TraderXCategory category)
	{
		ApplySelected();
	}

	void OnNavClick()
	{
		TraderXInventoryManager.PlayMenuSound(ETraderXSounds.QUICK_EVENT);
		Event_OnCategoryNavClick.Invoke(_inputCategory);
	}
}
