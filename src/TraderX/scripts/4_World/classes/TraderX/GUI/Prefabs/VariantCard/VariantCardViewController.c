class VariantCardViewController: ViewController
{
    ref TraderXProduct item;

    static ref ScriptInvoker Event_OnVariantClickCallBack = new ScriptInvoker();

    string variant_classname, variant_price;

    EntityAI preview;
    
    void Setup(TraderXProduct item)
    {
        this.item = item;

        variant_classname = item.GetDisplayName();
        variant_price = TraderXQuantityManager.GetFormattedMoneyAmount(item.buyPrice);

        preview = EntityAI.Cast(GetGame().CreateObjectEx(item.className, vector.Zero, ECE_LOCAL|ECE_NOLIFETIME));
        NotifyPropertiesChanged({"variant_classname", "variant_price", "preview"});
    }
    
    bool OnVariantExecute(ButtonCommandArgs args)
    {
        Event_OnVariantClickCallBack.Invoke(item);
        return true;
    }
}