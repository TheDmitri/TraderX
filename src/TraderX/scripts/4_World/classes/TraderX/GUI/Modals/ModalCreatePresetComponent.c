class ModalCreatePresetComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("tx-modal-preset", ModalCreatePresetComponent);

	static ref ScriptInvoker Event_OnConfirmClickCallBack = new ScriptInvoker();
	static ref ScriptInvoker Event_OnCancelClickCallBack = new ScriptInvoker();

	// Staged by the caller before ShowModal (the modal mounts via Component_Selector and
	// can't take constructor args). Lets the same modal serve create (empty name) and
	// edit/rename (prefilled name + different title).
	static string s_PendingName = "";
	static string s_PendingTitle = "#tpm_create_new_preset";

	string modal_title;
	string input_text;
	string error_text;

	EditBoxWidget nameInput;

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "tx-modal-preset";
		m_Metadata.templatePath = "TraderX/datasets/gui/TraderXPrefab/Modals/ModalCreatePresetView.layout";
	}

	override void OnInit()
	{
		AutoLoadWidgets();
		modal_title = CheckoutComponent.Upper(s_PendingTitle);
		input_text = s_PendingName;
		error_text = "";
		if (nameInput)
			nameInput.SetText(s_PendingName);
		MarkForCheck();
	}

	// Read the typed name straight from the widget — don't depend on the two-way
	// binding having written input_text back, which is the difference between the
	// preset being created and the confirm silently failing the length check.
	string GetEnteredName()
	{
		if (nameInput)
			return nameInput.GetText();
		return input_text;
	}

	void OnConfirmClick()
	{
		string name = GetEnteredName();
		if (name.Length() < 4 || name.Length() > 20)
		{
			error_text = "Preset name must be between 4 and 20 characters long";
			MarkForCheck();
			return;
		}

		error_text = "";
		MarkForCheck();
		Event_OnConfirmClickCallBack.Invoke(name);
	}

	void OnCancelClick()
	{
		Event_OnCancelClickCallBack.Invoke();
	}

	void OnInputTextChanged()
	{
		string name = GetEnteredName();
		if (name.Length() >= 4 && name.Length() <= 20)
		{
			error_text = "";
		}
		else
		{
			error_text = "Preset name must be between 4 and 20 characters long";
		}
		MarkForCheck();
	}
}
