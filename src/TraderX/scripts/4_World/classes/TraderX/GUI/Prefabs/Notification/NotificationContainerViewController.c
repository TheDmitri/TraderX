class NotificationContainerViewController extends ViewController
{
    protected ref ObservableCollection<ref NotificationCardView> notifications = new ObservableCollection<ref NotificationCardView>(this);

    static private NotificationContainerViewController m_instance;

    const int MAX_NOTIFICATIONS = 5;

    ref array<ref TraderXNotificationItem> notificationsQueue;

    void NotificationContainerViewController()
    {
        m_instance = this;
        notificationsQueue = new array<ref TraderXNotificationItem>();
        NotificationCardViewController.Event_OnNotificationFinished.Insert(OnNotificationFinished);
    }

    void OnNotificationFinished(NotificationCardView notificationView)
    {
        notifications.Remove(notificationView);
        CheckNotificationsQueue();
    }

    void CheckNotificationsQueue()
    {
        while(notifications.Count() < MAX_NOTIFICATIONS && notificationsQueue.Count() > 0)
        {
            TraderXNotificationItem notification = notificationsQueue[0];
            notificationsQueue.RemoveOrdered(0);
            notifications.Insert(NotificationCardView.CreateNotificationCardView(notification));
        }
    }

    void AddNotification(TraderXNotificationItem notification)
    {
        notificationsQueue.Insert(notification);
        CheckNotificationsQueue();
    }

    static NotificationContainerViewController GetInstance()
    {
        return m_instance;
    }
}