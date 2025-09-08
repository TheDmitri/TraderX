
class ModalConfirmationViewController: ViewController
{
    string title;
    string content;

    static ref ScriptInvoker Event_OnConfirmClickCallBack = new ScriptInvoker();
    static ref ScriptInvoker Event_OnCancelClickCallBack = new ScriptInvoker();

    void Setup(string title, string content)
    {
        this.title = title;
        this.content = content;
        GetLayoutRoot().Show(true);

        NotifyPropertyChanged("title");
        NotifyPropertyChanged("content");
    }

    bool OnConfirmExecute(ButtonCommandArgs args) 
    {
        Event_OnConfirmClickCallBack.Invoke();
        return true;
    }

    bool OnCancelExecute(ButtonCommandArgs args) 
    {
        Event_OnCancelClickCallBack.Invoke();
        return true;
    }
}