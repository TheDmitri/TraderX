class SellTooltipViewController: ViewController
{
    string product_description;
    string hierarchy;

    Widget leftRightClickPanel, quickSellPanel, shiftLeftClickPanel, shiftRightPanel, manualModePanel;

    void Setup(string productDescription, string productHierarchy)
    {
        this.product_description = productDescription;
        this.hierarchy = productHierarchy;

        if(!SellPageViewController.IsManualSell()){
            SelectionMode();
        }
        else{
            ManualMode();
        }

        NotifyPropertiesChanged({"product_description", "hierarchy"});
    }

    void ManualMode()
    {
        leftRightClickPanel.Show(false);
        shiftLeftClickPanel.Show(false);
        shiftRightPanel.Show(false);
        manualModePanel.Show(true);
        quickSellPanel.Show(true);
    }

    void SelectionMode()
    {
        leftRightClickPanel.Show(true);
        shiftLeftClickPanel.Show(true);
        shiftRightPanel.Show(true);
        manualModePanel.Show(false);
        quickSellPanel.Show(false);
    }
}