class TraderXLoadoutItem
{
    string className;
    int quantity;
    string slotName;
    ref array<ref TraderXLoadoutAttachment> attachments;

    void TraderXLoadoutItem(string className, int quantity = -1, string slotName = string.Empty)
    {
        this.className = className;
        this.quantity = quantity;
        this.slotName = slotName;

        attachments = new array<ref TraderXLoadoutAttachment>();
    }
}