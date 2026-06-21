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
}
