class NotificationDetailsComponent extends DZlrComponentBase
{
    private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("traderx-notification-details", NotificationDetailsComponent);

    bool m_Success;
    string m_Content, m_Status;
    TextWidget contentText, statusText;

    void NotificationDetailsComponent(bool success, string content, string status)
    {
        m_Success = success;
        m_Content = content;
        m_Status = status;
    }

    override protected void RegisterMetadata()
    {
        m_Metadata = new ComponentMetadata();
        m_Metadata.selector = "traderx-notification-details";
        m_Metadata.templatePath = "TraderX/datasets/gui/Notification/TraderXDetailedCard.layout";
    }

    override void OnInit()
    {
        if (contentText) contentText.SetText(m_Content);
        if (statusText) statusText.SetText(m_Status);

        if (m_Success)
        {
            if (contentText) contentText.SetColor(TraderXViewStyles.NOTIFICATION_SUCCESS);
            if (statusText) statusText.SetColor(TraderXViewStyles.NOTIFICATION_SUCCESS);
        }
        else
        {
            if (contentText) contentText.SetColor(TraderXViewStyles.NOTIFICATION_ERROR);
            if (statusText) statusText.SetColor(TraderXViewStyles.NOTIFICATION_ERROR);
        }
    }
}
