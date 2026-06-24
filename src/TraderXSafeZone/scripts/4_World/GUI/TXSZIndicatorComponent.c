/**
 * TXSZIndicatorComponent — persistent "in safe zone" HUD badge (Field Ledger).
 *
 * Discreet top-center badge shown the whole time the player stands inside a
 * safezone. Shield glyph (TraderXSafeZone/datasets/shield.paa, loaded in code)
 * + bound status text "<TITLE> · <ZONE>". DayZular component mounted directly on
 * the workspace (HUD overlay — no cursor, no pause); toggled by TXSZSafeZoneUI.
 *
 * Widget-name contract (must match data-name in TXSZIndicator.html):
 *   indicatorRoot, leftRule, shieldIcon, zoneText
 * Binding: {{ statusText }} (TextBinding) — never SetText(); MarkForCheck().
 */
class TXSZIndicatorComponent extends DZlrComponentBase
{
    // Bound text — "<TITLE> · <ZONE>".
    string statusText;

    // Widgets bound by name (AutoLoadWidgets).
    ImageWidget shieldIcon;

    override protected void RegisterMetadata()
    {
        m_Metadata = new ComponentMetadata();
        m_Metadata.selector = "txsz-indicator";
        m_Metadata.templatePath = "TraderXSafeZone/gui/TXSZIndicator.layout";
    }

    override void OnInit()
    {
        super.OnInit();

        if (shieldIcon)
        {
            shieldIcon.LoadImageFile(0, "TraderXSafeZone/datasets/shield.paa");
            shieldIcon.SetImage(0);
        }

        Hide();
    }

    void ShowZone(string title, string zoneName)
    {
        string head = TraderXSafeZoneTernary<string>.If(title != "", title, "SAFE ZONE");
        string zone = TraderXSafeZoneTernary<string>.If(zoneName != "", zoneName, "");

        statusText = TraderXSafeZoneTernary<string>.If(zone != "", head + " · " + zone, head);
        MarkForCheck();

        if (m_Root)
            m_Root.Show(true);
    }

    void Hide()
    {
        if (m_Root)
            m_Root.Show(false);
    }
}
