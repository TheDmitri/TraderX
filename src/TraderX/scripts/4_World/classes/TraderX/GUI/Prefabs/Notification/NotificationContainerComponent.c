class NotificationContainerComponent extends DZlrComponentBase
{
    private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("traderx-notification-container", NotificationContainerComponent);

    ref array<ref NotificationCardComponent> m_Notifications = new array<ref NotificationCardComponent>();
    static private NotificationContainerComponent m_instance;
    const int MAX_NOTIFICATIONS = 5;
    ref array<ref TraderXNotificationItem> notificationsQueue;
    Widget notificationsPanel;

    override protected void RegisterMetadata()
    {
        m_Metadata = new ComponentMetadata();
        m_Metadata.selector = "traderx-notification-container";
        m_Metadata.templatePath = "TraderX/datasets/gui/Notification/TraderXNotificationParent.layout";
    }

    override void OnInit()
    {
        m_instance = this;
        notificationsQueue = new array<ref TraderXNotificationItem>();
        NotificationCardComponent.Event_OnNotificationFinished.Insert(OnNotificationFinished);
    }

    override void OnDestroy()
    {
        NotificationCardComponent.Event_OnNotificationFinished.Remove(OnNotificationFinished);
        m_instance = null;
    }

    static NotificationContainerComponent GetInstance() { return m_instance; }

    void OnNotificationFinished(NotificationCardComponent card)
    {
        int idx = m_Notifications.Find(card);
        if (idx != -1)
        {
            if (card.GetRoot()) card.GetRoot().Unlink();
            m_Notifications.Remove(idx);
        }
        CheckNotificationsQueue();
    }

    void CheckNotificationsQueue()
    {
        while (m_Notifications.Count() < MAX_NOTIFICATIONS && notificationsQueue.Count() > 0)
        {
            TraderXNotificationItem notification = notificationsQueue[0];
            notificationsQueue.RemoveOrdered(0);
            NotificationCardComponent card = NotificationCardComponent.CreateNotificationCard(notification);
            card.InitComponent(null);
            if (notificationsPanel) { notificationsPanel.AddChild(card.GetRoot()); card.GetRoot().Show(true); }
            m_Notifications.Insert(card);
        }
    }

    void AddNotification(TraderXNotificationItem notification)
    {
        notificationsQueue.Insert(notification);
        CheckNotificationsQueue();
    }
}
