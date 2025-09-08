class CustomizeTooltipViewController: ViewController
{
    string product_description;

    void Setup(string productDescription)
    {        
        this.product_description = productDescription;
        NotifyPropertyChanged("product_description");
    }
}