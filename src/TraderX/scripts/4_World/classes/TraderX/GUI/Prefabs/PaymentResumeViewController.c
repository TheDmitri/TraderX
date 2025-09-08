class PaymentResumeViewController: ViewController
{
    string payment_amount;
    string bank_account;
    string wallet_account;
    
    void PaymentResumeViewController()
    {

    }

    bool OnPurchaseExecute(ButtonCommandArgs args)
	{
        //Purchase
		return true;
	}
}