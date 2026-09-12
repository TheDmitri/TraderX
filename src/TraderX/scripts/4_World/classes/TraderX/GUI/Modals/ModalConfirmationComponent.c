class ModalConfirmationComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("tx-modal-confirm", ModalConfirmationComponent);

	static ref ScriptInvoker Event_OnConfirmClickCallBack = new ScriptInvoker();
	static ref ScriptInvoker Event_OnCancelClickCallBack = new ScriptInvoker();

	// When the modal is mounted via ComponentWidgetBinding (Component_Selector) the
	// caller cannot pass constructor args, so it stages the text here first.
	static string s_PendingTitle = "";
	static string s_PendingContent = "";

	string title;
	string content;

	// Standalone overlay instance (mounted over an already-open trader menu, like the promo
	// modal) so any component can raise a confirm dialog without owning a modal host.
	protected static ref ModalConfirmationComponent s_Overlay;

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "tx-modal-confirm";
		m_Metadata.templatePath = "TraderX/datasets/gui/TraderXPrefab/Modals/ModalConfirmationView.layout";
	}

	override void OnInit()
	{
		if (s_PendingTitle != "" || s_PendingContent != "")
		{
			title = s_PendingTitle;
			content = s_PendingContent;
			MarkForCheck();
		}
	}

	void Setup(string modalTitle, string modalContent)
	{
		title = modalTitle;
		content = modalContent;
		GetRoot().Show(true);
		MarkForCheck();
	}

	void OnConfirmClick()
	{
		Event_OnConfirmClickCallBack.Invoke();
	}

	void OnCancelClick()
	{
		Event_OnCancelClickCallBack.Invoke();
	}

	// Mount the confirm dialog as a full-screen overlay. Idempotent — an existing overlay is
	// closed first. Listen on Event_OnConfirmClickCallBack / Event_OnCancelClickCallBack for
	// the choice, and call Close() when done.
	static void Open(string modalTitle, string modalContent)
	{
		Close();

		s_PendingTitle = modalTitle;
		s_PendingContent = modalContent;

		s_Overlay = new ModalConfirmationComponent();
		s_Overlay.InitComponent(null);

		Widget root = s_Overlay.GetRoot();
		if (!root)
		{
			s_Overlay = null;
			s_PendingTitle = "";
			s_PendingContent = "";
			return;
		}

		s_Overlay.title = modalTitle;
		s_Overlay.content = modalContent;
		s_Overlay.MarkForCheck();
		root.Show(true);
		GetGame().GetUIManager().ShowUICursor(true);
	}

	static void Close()
	{
		s_PendingTitle = "";
		s_PendingContent = "";
		if (s_Overlay)
		{
			s_Overlay.Destroy();
			s_Overlay = null;
		}
	}
}
