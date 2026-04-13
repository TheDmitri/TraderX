class ModalConfirmationComponent extends DZlrComponentBase
{
    private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("traderx-modal-confirm", ModalConfirmationComponent);

    static ref ScriptInvoker Event_OnConfirmClickCallBack = new ScriptInvoker();
    static ref ScriptInvoker Event_OnCancelClickCallBack = new ScriptInvoker();

    string m_Title, m_Content;
    TextWidget titleText, contentText;
    ButtonWidget confirmBtn, cancelBtn;

    void ModalConfirmationComponent(string title, string content)
    {
        m_Title = title;
        m_Content = content;
    }

    override protected void RegisterMetadata()
    {
        m_Metadata = new ComponentMetadata();
        m_Metadata.selector = "traderx-modal-confirm";
        m_Metadata.templatePath = "TraderX/datasets/gui/TraderXPrefab/Modals/ModalConfirmationView.layout";
    }

    override void OnInit()
    {
        if (titleText) titleText.SetText(m_Title);
        if (contentText) contentText.SetText(m_Content);
        if (m_Root) m_Root.Show(true);
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (w == confirmBtn)
        {
            Event_OnConfirmClickCallBack.Invoke();
            return true;
        }
        if (w == cancelBtn)
        {
            Event_OnCancelClickCallBack.Invoke();
            return true;
        }
        return false;
    }

    override bool OnMouseEnter(Widget w, int x, int y)
    {
        if (w && w.GetUserID() == 333)
            w.SetAlpha(1.0);
        return false;
    }

    override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
    {
        if (w && w.GetUserID() == 333)
            w.SetAlpha(0.70);
        return false;
    }
}
