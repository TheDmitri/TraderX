// Reusable list/grid view switch (Nasdara segment: [grid | list], the active half filled).
// Embed once per page via data-component="tx-view-toggle". Sets the global
// TraderXViewModeService; every product list reacts on its own.
class TraderXViewToggleComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("tx-view-toggle", TraderXViewToggleComponent);

	ImageWidget listIcon;
	ImageWidget gridIcon;
	Widget gridBg;
	Widget listBg;

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "tx-view-toggle";
		m_Metadata.templatePath = "TraderX/datasets/gui/TraderXPrefab/Navigation/ViewToggleView.layout";
	}

	override void OnInit()
	{
		AutoLoadWidgets();
		TraderXViewModeService.Event_OnViewModeChangedCallBack.Insert(OnViewModeChanged);
		UpdateIcon();
	}

	override void OnDestroy()
	{
		TraderXViewModeService.Event_OnViewModeChangedCallBack.Remove(OnViewModeChanged);
	}

	void OnGridClick()
	{
		if (!TraderXViewModeService.GetInstance().IsGridView())
			TraderXViewModeService.GetInstance().Toggle();
	}

	void OnListClick()
	{
		if (TraderXViewModeService.GetInstance().IsGridView())
			TraderXViewModeService.GetInstance().Toggle();
	}

	// Kept for layouts that still use one toggle button.
	void OnViewToggleClick()
	{
		TraderXViewModeService.GetInstance().Toggle();
	}

	void OnViewModeChanged(bool grid)
	{
		UpdateIcon();
	}

	void UpdateIcon()
	{
		bool grid = TraderXViewModeService.GetInstance().IsGridView();
		if (gridBg) gridBg.Show(grid);
		if (listBg) listBg.Show(!grid);
		if (gridIcon) gridIcon.SetColor(DZTernaryInt.If(grid, TraderXTheme.NAV_ACTIVE, TraderXTheme.NAV_INACTIVE));
		if (listIcon) listIcon.SetColor(DZTernaryInt.If(grid, TraderXTheme.NAV_INACTIVE, TraderXTheme.NAV_ACTIVE));
	}
}
