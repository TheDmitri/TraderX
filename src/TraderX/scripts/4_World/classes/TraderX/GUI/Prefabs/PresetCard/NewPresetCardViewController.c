class NewPresetCardViewController: ViewController
{
    static ref ScriptInvoker Event_OnNewPresetClickCallBack = new ScriptInvoker();

    bool OnNewPresetExecute(ButtonCommandArgs args)
    {
        Event_OnNewPresetClickCallBack.Invoke();
        return true;
    }
}