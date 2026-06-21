// Reusable list/grid view-toggle button. Embed once per page via
// data-component="tx-view-toggle". Flips the global TraderXViewModeService;
// every product list reacts on its own. No per-page logic duplication.
class TraderXViewToggleComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("tx-view-toggle", TraderXViewToggleComponent);

	ImageWidget listIcon;
	ImageWidget gridIcon;
	ButtonWidget viewToggleBtn;
	TextWidget viewLabel;

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
		// Show the icon + label of the mode you can switch TO.
		if (gridIcon) gridIcon.Show(!grid);
		if (listIcon) listIcon.Show(grid);
		if (viewLabel)
		{
			if (grid)
				viewLabel.SetText("List");
			else
				viewLabel.SetText("Grid");
		}
	}
}
