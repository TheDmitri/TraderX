class NotificationCardComponent extends DZlrComponentBase
{
    private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("traderx-notification-card", NotificationCardComponent);

    static ref ScriptInvoker Event_OnNotificationFinished = new ScriptInvoker();

    ref TraderXNotificationItem m_Notification;
    string notification_title, transaction_result_text, details_text;
    Widget detailsPanel, detailsGrid, notificationProgressBar;
    TextWidget notificationTitleText, transactionResultText;
    ButtonWidget closeBtn, detailsBtn;
    ref array<ref NotificationDetailsComponent> m_DetailsList = new array<ref NotificationDetailsComponent>();
    bool isDetailsPanelOpen = false;
    int creationTime, duration;

    protected string m_LayoutPath = "TraderX/datasets/gui/Notification/TraderXNotificationInfoCard.layout";

    void NotificationCardComponent(TraderXNotificationItem notificationItem)
    {
        m_Notification = notificationItem;
    }

    static NotificationCardComponent CreateNotificationCard(TraderXNotificationItem notificationItem)
    {
        NotificationCardComponent card;
        switch (notificationItem.notificationType)
        {
            case ENotificationType.ERROR:
                card = new NotificationCardComponent(notificationItem);
                card.m_LayoutPath = "TraderX/datasets/gui/Notification/TraderXNotificationErrorCard.layout";
                break;
            case ENotificationType.SUCCESS:
                card = new NotificationCardComponent(notificationItem);
                card.m_LayoutPath = "TraderX/datasets/gui/Notification/TraderXNotificationSuccessCard.layout";
                break;
            case ENotificationType.WARNING:
                card = new NotificationCardComponent(notificationItem);
                card.m_LayoutPath = "TraderX/datasets/gui/Notification/TraderXNotificationWarningCard.layout";
                break;
            default:
                card = new NotificationCardComponent(notificationItem);
                break;
        }
        return card;
    }

    override protected void RegisterMetadata()
    {
        m_Metadata = new ComponentMetadata();
        m_Metadata.selector = "traderx-notification-card";
        m_Metadata.templatePath = m_LayoutPath;
    }

    override void OnInit()
    {
        creationTime = GetGame().GetTime() / 1000;
        duration = m_Notification.duration;

        notification_title = m_Notification.notificationTitle;
        transaction_result_text = m_Notification.content;

        if (notificationTitleText) notificationTitleText.SetText(notification_title);
        if (transactionResultText) transactionResultText.SetText(transaction_result_text);

        if (m_Notification.hasDetails)
        {
            if (detailsPanel) detailsPanel.Show(true);
            details_text = m_Notification.details;
        }
        else
        {
            if (detailsPanel) detailsPanel.Show(false);
        }

        StartProgressBarAnimation();
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (w == closeBtn)
        {
            if (m_Root) m_Root.Show(false);
            Event_OnNotificationFinished.Invoke(this);
            return true;
        }
        if (w == detailsBtn)
        {
            ToggleDetailsPanel();
            return true;
        }
        return false;
    }

    void ToggleDetailsPanel()
    {
        if (!isDetailsPanelOpen)
        {
            if (detailsGrid) detailsGrid.Show(true);
            creationTime = GetGame().GetTime() / 1000;
            duration = 30;
        }
        else
        {
            if (detailsGrid) detailsGrid.Show(false);
            creationTime = GetGame().GetTime() / 1000;
            duration = m_Notification.duration;
        }
        isDetailsPanelOpen = !isDetailsPanelOpen;
    }

    void StartProgressBarAnimation()
    {
        if (!notificationProgressBar) return;
        notificationProgressBar.SetSize(1, 1);
        GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(UpdateProgressBar, 25, true);
    }

    void UpdateProgressBar()
    {
        if (!notificationProgressBar) return;
        float elapsedTime = (GetGame().GetTime() / 1000) - creationTime;
        if (duration <= 0) duration = 1;
        float remainingPercentage = 1.0 - (elapsedTime / duration);
        if (remainingPercentage < 0) remainingPercentage = 0;
        notificationProgressBar.SetSize(remainingPercentage, 1);

        if (remainingPercentage <= 0)
        {
            GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(UpdateProgressBar);
            if (m_Root) m_Root.Show(false);
            Event_OnNotificationFinished.Invoke(this);
        }
    }
}
