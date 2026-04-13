class PaymentResumeComponent extends DZlrComponentBase
{
    private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("traderx-payment-resume", PaymentResumeComponent);

    string payment_amount, bank_account, wallet_account;
    ButtonWidget purchaseBtn;

    override protected void RegisterMetadata()
    {
        m_Metadata = new ComponentMetadata();
        m_Metadata.selector = "traderx-payment-resume";
        m_Metadata.templatePath = "TraderX/datasets/gui/TraderXPrefab/PaymentResumeView.layout";
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (w == purchaseBtn) return true;
        return false;
    }

    override bool OnMouseEnter(Widget w, int x, int y)
    {
        if (w && w.GetUserID() == EButtonIds.PURCHASE) {}
        return false;
    }

    override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
    {
        if (w && w.GetUserID() == EButtonIds.PURCHASE) {}
        return false;
    }
}
