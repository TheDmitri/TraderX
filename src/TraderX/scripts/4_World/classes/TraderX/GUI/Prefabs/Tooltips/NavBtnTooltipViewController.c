class NavBtnTooltipViewController: ViewController
{
    string title;

    void Setup(string title)
    {
        this.title = title;
        NotifyPropertyChanged("title");
    }
}