class ModalCreatePresetComponent extends DZlrComponentBase
{
    private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("traderx-modal-create-preset", ModalCreatePresetComponent);

    static ref ScriptInvoker Event_OnConfirmClickCallBack = new ScriptInvoker();
    static ref ScriptInvoker Event_OnCancelClickCallBack = new ScriptInvoker();

    string input_text;
    string error_text;

    EditBoxWidget inputBox;
    TextWidget errorText;
    ButtonWidget confirmBtn, cancelBtn;

    override protected void RegisterMetadata()
    {
        m_Metadata = new ComponentMetadata();
        m_Metadata.selector = "traderx-modal-create-preset";
        m_Metadata.templatePath = "TraderX/datasets/gui/TraderXPrefab/Modals/ModalCreatePresetView.layout";
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (w == confirmBtn)
        {
            if (inputBox) input_text = inputBox.GetText();

            if (input_text.Length() < 4 || input_text.Length() > 20)
            {
                error_text = "Preset name must be between 4 and 20 characters long";
                if (errorText) errorText.SetText(error_text);
                return false;
            }

            error_text = "";
            if (errorText) errorText.SetText(error_text);
            Event_OnConfirmClickCallBack.Invoke(input_text);
            return true;
        }
        if (w == cancelBtn)
        {
            Event_OnCancelClickCallBack.Invoke();
            return true;
        }
        return false;
    }

    override bool OnChange(Widget w, int x, int y, bool finished)
    {
        if (w == inputBox)
        {
            input_text = inputBox.GetText();
            if (input_text.Length() >= 4 && input_text.Length() <= 20)
                error_text = "";
            else
                error_text = "Preset name must be between 4 and 20 characters long";

            if (errorText) errorText.SetText(error_text);
            return true;
        }
        return false;
    }
}
