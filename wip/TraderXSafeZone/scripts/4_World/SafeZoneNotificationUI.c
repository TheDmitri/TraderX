class TraderXSafeZoneNotificationUI extends UIScriptedMenu
{
    private Widget m_RootWidget;
    private RichTextWidget m_NotifText;

    private int m_countdown;
    private int m_startcountdown;
    private string m_msg;
    private string m_notifTitle;
    private bool m_useOverlay = false;

    void TraderXSafeZoneNotificationUI()
    {
        m_RootWidget = GetGame().GetWorkspace().CreateWidgets("TraderXSafeZone/gui/SafeZoneNotification.layout");
        if (!m_RootWidget)
        {
            Print("[TraderXSafeZone] TraderXSafeZoneNotificationUI: Failed to load layout");
            return;
        }
        
        m_NotifText = RichTextWidget.Cast(m_RootWidget.FindAnyWidget("NotifText"));
        if (m_NotifText)
        {
            m_useOverlay = true;
            m_RootWidget.Show(true);
        }
    }

    void Delete()
    {
        GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(CountdownUpdate);

        if (m_RootWidget)
            m_RootWidget.Show(false);
    }

    void ~TraderXSafeZoneNotificationUI()
    {
        GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(CountdownUpdate);
    }

    void StartCountdown(int i, string msgCustom, string notifTitle)
    {
        m_notifTitle = TraderXSafeZoneTernary<string>.If(notifTitle != "", notifTitle, "SafeZone");
        m_msg = TraderXSafeZoneTernary<string>.If(msgCustom != "", msgCustom, "You will leave the safe zone in %1 seconds");
        
        m_countdown = i;
        m_startcountdown = i;
        string text = GetCountdownText();

        if (m_useOverlay && m_NotifText){
            m_NotifText.SetText(text);
        }
        else{
            NotificationSystem.AddNotificationExtended(1, m_notifTitle, text, "");
        }

        GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(CountdownUpdate, 1000, true);
    }

    string GetCountdownText()
    {
        if (m_msg.IndexOf("%1") >= 0){
            return string.Format(m_msg, m_countdown);
        }
        
        return m_msg + m_countdown.ToString();
    }

    void CountdownUpdate()
    {
        m_countdown--;
        string text = GetCountdownText();

        if (m_useOverlay && m_NotifText){
            m_NotifText.SetText(text);
        }
        else{
            NotificationSystem.AddNotificationExtended(1, m_notifTitle, text, "");
        }

        if (m_countdown <= 0)
        {
            GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(CountdownUpdate);
        }
    }
}
