/**
 * TXSZCountdownComponent — leave-zone countdown (Field Ledger), replaces the raw
 * SafeZoneNotification.layout + UIScriptedMenu. A single bound text line that
 * ticks every second via DayZular TextBinding ({{ countdownText }}) — never
 * SetText(); MarkForCheck() drives the redraw. DayZular component mounted on the
 * workspace; toggled/ticked by TXSZSafeZoneUI through the SafeZoneHandler hooks.
 *
 * Widget-name contract (must match data-name in TXSZCountdown.html):
 *   countdownRoot, leftRule, countdownLabel
 * Binding: {{ countdownText }} (TextBinding).
 */
class TXSZCountdownComponent extends DZlrComponentBase
{
    // Bound text — formatted countdown line.
    string countdownText;

    private int m_Countdown;
    private string m_MsgTemplate;

    override protected void RegisterMetadata()
    {
        m_Metadata = new ComponentMetadata();
        m_Metadata.selector = "txsz-countdown";
        m_Metadata.templatePath = "TraderXSafeZone/gui/TXSZCountdown.layout";
    }

    override void OnInit()
    {
        super.OnInit();
        Hide();
    }

    void StartCountdown(int seconds, string msgTemplate)
    {
        m_Countdown = seconds;
        m_MsgTemplate = TraderXSafeZoneTernary<string>.If(msgTemplate != "", msgTemplate, "You are leaving the safezone in %1 seconds");

        RefreshText();

        if (m_Root)
            m_Root.Show(true);

        GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(Tick);
        GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(Tick, 1000, true);
    }

    private void Tick()
    {
        m_Countdown--;
        RefreshText();

        if (m_Countdown <= 0)
            GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(Tick);
    }

    private void RefreshText()
    {
        if (m_MsgTemplate.IndexOf("%1") >= 0)
            countdownText = string.Format(m_MsgTemplate, m_Countdown);
        else
            countdownText = m_MsgTemplate + m_Countdown.ToString();

        MarkForCheck();
    }

    void Stop()
    {
        GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(Tick);
        Hide();
    }

    void Hide()
    {
        if (m_Root)
            m_Root.Show(false);
    }

    void ~TXSZCountdownComponent()
    {
        GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(Tick);
    }
}
