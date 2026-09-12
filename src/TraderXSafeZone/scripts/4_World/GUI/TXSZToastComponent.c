/**
 * TXSZToastComponent — styled enter/exit toast (Field Ledger), replaces the raw
 * NotificationSystem cards. One reusable top-right card: shield glyph + title +
 * body, with a left accent rule recolored per transition (olive = enter, rust =
 * exit). Auto-hides after a timer. DayZular component mounted on the workspace.
 *
 * Widget-name contract (must match data-name in TXSZToast.html):
 *   toastRoot, leftRule, iconWell, shieldIcon, titleText, bodyText
 * Bindings: {{ toastTitle }}, {{ toastBody }} (TextBinding) — MarkForCheck().
 */
class TXSZToastComponent extends DZlrComponentBase
{
    // Bound text.
    string toastTitle;
    string toastBody;

    // Widgets bound by name (AutoLoadWidgets).
    Widget leftRule;
    ImageWidget shieldIcon;

    // Field Ledger accents: olive (positive entry) / rust (inverse exit).
    int GetEnterAccent() { return ARGB(255, 122, 154, 74); }
    int GetExitAccent()  { return ARGB(255, 176, 72, 42); }

    override protected void RegisterMetadata()
    {
        m_Metadata = new ComponentMetadata();
        m_Metadata.selector = "txsz-toast";
        m_Metadata.templatePath = "TraderXSafeZone/gui/TXSZToast.layout";
    }

    override void OnInit()
    {
        super.OnInit();
        Hide();
    }

    void ShowToast(string title, string body, bool isEnter, int seconds)
    {
        toastTitle = TraderXSafeZoneTernary<string>.If(title != "", title, "SafeZone");
        toastBody = body;
        MarkForCheck();

        int accent = TraderXSafeZoneTernary<int>.If(isEnter, GetEnterAccent(), GetExitAccent());
        if (leftRule)
            leftRule.SetColor(accent);
        if (shieldIcon)
            shieldIcon.SetColor(accent);

        if (m_Root)
            m_Root.Show(true);

        int ms = seconds * 1000;
        if (ms <= 0)
            ms = 4000;

        GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(Hide);
        GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(Hide, ms);
    }

    void Hide()
    {
        if (m_Root)
            m_Root.Show(false);
    }

    void ~TXSZToastComponent()
    {
        GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(Hide);
    }
}
