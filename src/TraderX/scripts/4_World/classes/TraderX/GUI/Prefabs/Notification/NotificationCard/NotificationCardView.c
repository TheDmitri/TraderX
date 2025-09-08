class NotificationCardView : ScriptViewTemplate<NotificationCardViewController>
{
    void NotificationCardView(TraderXNotificationItem notificationItem)
    {
        m_TemplateController.Setup(this, notificationItem);
    }

    static NotificationCardView CreateNotificationCardView(TraderXNotificationItem notificationItem)
	{
        switch(notificationItem.notificationType)
        {
            case ENotificationType.INFO:
                return new NotificationInfoCardView(notificationItem);
            case ENotificationType.ERROR:
                return new NotificationErrorCardView(notificationItem);
            case ENotificationType.SUCCESS:
                return new NotificationSuccessCardView(notificationItem);
            case ENotificationType.WARNING:
                return new NotificationWarningCardView(notificationItem);
        }
		return new NotificationCardView(notificationItem);
	}

    override string GetLayoutFile()
    {
        return "TraderX/datasets/gui/Notification/TraderXNotificationInfoCard.layout";
    }
}

class NotificationInfoCardView : NotificationCardView
{
    void NotificationInfoCardView(TraderXNotificationItem notificationItem)
    {
        m_TemplateController.Setup(this, notificationItem);
    }

    override string GetLayoutFile()
    {
        return "TraderX/datasets/gui/Notification/TraderXNotificationInfoCard.layout";
    }
}

class NotificationErrorCardView : NotificationCardView
{

    void NotificationErrorCardView(TraderXNotificationItem notificationItem)
    {
        m_TemplateController.Setup(this, notificationItem);
    }
    override string GetLayoutFile()
    {
        return "TraderX/datasets/gui/Notification/TraderXNotificationErrorCard.layout";
    }
}

class NotificationSuccessCardView : NotificationCardView
{
    void NotificationSuccessCardView(TraderXNotificationItem notificationItem)
    {
        m_TemplateController.Setup(this, notificationItem);
    }
    override string GetLayoutFile()
    {
        return "TraderX/datasets/gui/Notification/TraderXNotificationSuccessCard.layout";
    }
}

class NotificationWarningCardView : NotificationCardView
{
    void NotificationWarningCardView(TraderXNotificationItem notificationItem)
    {
        m_TemplateController.Setup(this, notificationItem);
    }
    override string GetLayoutFile()
    {
        return "TraderX/datasets/gui/Notification/TraderXNotificationWarningCard.layout";
    }
}
