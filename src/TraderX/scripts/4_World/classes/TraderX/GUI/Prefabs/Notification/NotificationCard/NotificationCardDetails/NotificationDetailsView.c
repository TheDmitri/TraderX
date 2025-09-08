class NotificationDetailsView: ScriptViewTemplate<NotificationDetailsViewController>
{
    TextWidget contentText;
    TextWidget statusText;

    void NotificationDetailsView(bool success, string content, string status)
    {
        contentText.SetText(content);
        statusText.SetText(status);

        if(success)
        {
            contentText.SetColor(TraderXViewStyles.NOTIFICATION_SUCCESS);
            statusText.SetColor(TraderXViewStyles.NOTIFICATION_SUCCESS);
        }
        else
        {
            contentText.SetColor(TraderXViewStyles.NOTIFICATION_ERROR);
            statusText.SetColor(TraderXViewStyles.NOTIFICATION_ERROR);
        }
    }
    
    override string GetLayoutFile()
    {
        return "TraderX/datasets/gui/Notification/TraderXDetailedCard.layout";
    }
}