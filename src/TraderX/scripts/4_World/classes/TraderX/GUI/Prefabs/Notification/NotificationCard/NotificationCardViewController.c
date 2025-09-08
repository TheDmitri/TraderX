class NotificationCardViewController extends ViewController
{
    ref TraderXNotificationItem m_Notification;

    string notification_title;
    string transaction_result_text;
    string details_text;

    Widget detailsPanel, detailsGrid;

    ref ObservableCollection<ref ScriptView> notification_details_list = new ObservableCollection<ref ScriptView>(this);
    
    Widget notificationProgressBar;

    bool isDetailsPanelOpen = false;

    int creationTime;
    int duration;

    ref NotificationCardView m_View;

    static ref ScriptInvoker Event_OnNotificationFinished = new ScriptInvoker();

    ref TraderXNotificationItem GetNotification()
    {
        return m_Notification;
    }

    void Setup(NotificationCardView view, TraderXNotificationItem notification)
    {
        creationTime = GetGame().GetTime()/1000;
        duration = notification.duration;

        m_View = view;
        m_Notification = notification;
        
        notification_title = notification.notificationTitle;
        transaction_result_text = notification.content;

        if(notification.hasDetails){
            detailsPanel.Show(true);
            details_text = notification.details;
            foreach(TraderXNotificationDetails detail: notification.detailsList){
                NotificationDetailsView detailsView = new NotificationDetailsView(detail.success, detail.content, detail.status);
                notification_details_list.Insert(detailsView);
            }
            NotifyPropertyChanged("details_text");
        }else{
            detailsPanel.Show(false);
        }

        NotifyPropertiesChanged({"notification_title", "transaction_result_text"});

        StartProgressBarAnimation();
    }

    void ToggleDetailsPanel()
    {
        if(!isDetailsPanelOpen){
            detailsGrid.Show(true);
            creationTime = GetGame().GetTime()/1000;
            duration = 30;
            notification_details_list.Clear();
            foreach(TraderXNotificationDetails detail: m_Notification.detailsList){
                notification_details_list.Insert(new NotificationDetailsView(detail.success, detail.content, detail.status));
            }
        }else{
            detailsGrid.Show(false);
            creationTime = GetGame().GetTime()/1000;
            duration = m_Notification.duration;
            notification_details_list.Clear();
        }
        isDetailsPanelOpen = !isDetailsPanelOpen;
    }

    bool OnCloseExecute(ButtonCommandArgs args)
	{
        Show(false);
        Event_OnNotificationFinished.Invoke(m_View);
		return true;
	}

    bool OnDetailsExecute(ButtonCommandArgs args)
	{
        ToggleDetailsPanel();
		return true;
	}

    void StartProgressBarAnimation()
    {
        if (!notificationProgressBar)
            return;
            
        // Set initial width to 100%
        notificationProgressBar.SetSize(1, 1);
        
        // Start animation timer
        GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(UpdateProgressBar, 25, true);
    }
    
    void UpdateProgressBar()
    {
        if (!notificationProgressBar)
            return;
            
        // Get elapsed time since notification was created
        float elapsedTime = (GetGame().GetTime()/1000) - creationTime;

        if(duration <= 0)
            duration = 1;
        
        // Calculate remaining width percentage
        float remainingPercentage = 1.0 - (elapsedTime / duration);
        
        // Ensure it doesn't go below 0
        if (remainingPercentage < 0)
            remainingPercentage = 0;
            
        // Update progress bar width
        notificationProgressBar.SetSize(remainingPercentage, 1);
        
        // Stop animation if duration is reached
        if (remainingPercentage <= 0)
        {
            GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(UpdateProgressBar);
            Show(false);
            Event_OnNotificationFinished.Invoke(m_View);
        }
    }
}
