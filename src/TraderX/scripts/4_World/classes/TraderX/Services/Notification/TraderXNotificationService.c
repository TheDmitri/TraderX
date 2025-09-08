class TraderXNotificationService
{
    private static ref TraderXNotificationService m_Instance;
    private ref NotificationContainerView m_ContainerView;
    
    static TraderXNotificationService GetInstance()
    {
        if (!m_Instance)
        {
            m_Instance = new TraderXNotificationService();
        }
        return m_Instance;
    }
    
    void TraderXNotificationService()
    {
        m_ContainerView = new NotificationContainerView();
    }

    void ShowTransactionNotification(int notificationType, int duration, string title, string content, bool hasMultipleTransactions, string detailsText, array<ref TraderXNotificationDetails> details)
    {
        TraderXNotificationItem notificationItem = new TraderXNotificationItem(notificationType, duration, title, content, hasMultipleTransactions, detailsText, details);
        m_ContainerView.GetTemplateController().AddNotification(notificationItem);
    }
}
