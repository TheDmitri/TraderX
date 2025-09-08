class ModalCreatePresetViewController : ViewController
{
    string input_text, error_text;

    static ref ScriptInvoker Event_OnConfirmClickCallBack = new ScriptInvoker();
    static ref ScriptInvoker Event_OnCancelClickCallBack = new ScriptInvoker();

    bool OnConfirmExecute(ButtonCommandArgs args) 
    {
        if(input_text.Length() < 4 || input_text.Length() > 20){
            error_text = "Preset name must be between 4 and 20 characters long";
            NotifyPropertyChanged("error_text");
            return false;
        }

        error_text = "";
        NotifyPropertyChanged("error_text");
        Event_OnConfirmClickCallBack.Invoke(input_text);
        return true;
    }

    bool OnCancelExecute(ButtonCommandArgs args) 
    {
        Event_OnCancelClickCallBack.Invoke();
        return true;
    }

    override void PropertyChanged(string property_name)
	{
		switch (property_name)
		{
			case "input_text": 
			{
				if(input_text.Length() > 4 && input_text.Length() < 20)
				{
					error_text = "";
				}
				else
				{
					error_text = "Preset name must be between 4 and 20 characters long";
				}
                NotifyPropertyChanged("error_text");
				break;
			}
		}
	}
}
