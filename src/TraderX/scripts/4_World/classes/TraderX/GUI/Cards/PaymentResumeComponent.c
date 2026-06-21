class PaymentResumeComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("tx-payment-resume", PaymentResumeComponent);

	string payment_amount;
	string bank_account;
	string wallet_account;

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "tx-payment-resume";
		m_Metadata.templatePath = "TraderX/datasets/gui/TraderXPrefab/PaymentResumeView.layout";
	}

	void OnPurchaseClick()
	{
		// Purchase action
	}
}
