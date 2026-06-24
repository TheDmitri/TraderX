/**
 * TXSZSafeZoneUI — client-side UI controller for the safezone HUD.
 *
 * Owns the three DayZular components (persistent indicator, enter/exit toast,
 * leave countdown) and exposes a small API the SafeZoneHandler branches onto its
 * existing hooks (OnEnterSafeZone / StartOnLeaving / OnExitSafeZone). Replaces the
 * old UIScriptedMenu + raw NotificationSystem path. Components mount lazily on the
 * workspace the first time the controller is used (client only).
 */
class TXSZSafeZoneUI
{
    private ref TXSZIndicatorComponent m_Indicator;
    private ref TXSZToastComponent m_Toast;
    private ref TXSZCountdownComponent m_Countdown;
    private bool m_Built = false;

    void Build()
    {
        if (m_Built)
            return;
        if (!GetGame() || !GetGame().IsClient())
            return;

        m_Indicator = new TXSZIndicatorComponent();
        m_Indicator.InitComponent(null);

        m_Toast = new TXSZToastComponent();
        m_Toast.InitComponent(null);

        m_Countdown = new TXSZCountdownComponent();
        m_Countdown.InitComponent(null);

        m_Built = true;
    }

    // Player entered a zone: persistent indicator on + entry toast.
    void OnEnter(string title, string body, string zoneName, int seconds)
    {
        Build();

        if (m_Countdown)
            m_Countdown.Stop();
        if (m_Indicator)
            m_Indicator.ShowZone(title, zoneName);
        if (m_Toast)
            m_Toast.ShowToast(title, body, true, seconds);
    }

    // Player left a zone: countdown off, indicator off + exit toast.
    void OnExit(string title, string body, int seconds)
    {
        Build();

        if (m_Countdown)
            m_Countdown.Stop();
        if (m_Indicator)
            m_Indicator.Hide();
        if (m_Toast)
            m_Toast.ShowToast(title, body, false, seconds);
    }

    void StartCountdown(int seconds, string msgTemplate)
    {
        Build();

        if (m_Countdown)
            m_Countdown.StartCountdown(seconds, msgTemplate);
    }

    void StopCountdown()
    {
        if (m_Countdown)
            m_Countdown.Stop();
    }
}
