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

    // True while the player is inside a zone. The badge is a HUD overlay, so it also gets
    // suppressed whenever a menu is open (it sits top-center and collided with the trader
    // menu / inventory). A periodic tick reconciles visibility = inZone && no menu open.
    private bool m_InZone;

    override protected void RegisterMetadata()
    {
        m_Metadata = new ComponentMetadata();
        m_Metadata.selector = "txsz-indicator";
        m_Metadata.templatePath = "TraderXSafeZone/gui/TXSZIndicator.layout";
    }

    override void OnInit()
    {
        super.OnInit();
        m_InZone = false;
        if (m_Root)
            m_Root.Show(false);
        // Re-check menu state a few times a second so the badge hides/re-shows with menus.
        GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(ApplyVisibility, 250, true);
    }

    override void OnDestroy()
    {
        GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(ApplyVisibility);
        super.OnDestroy();
    }

    void ShowZone(string title, string zoneName)
    {
        string head = TraderXSafeZoneTernary<string>.If(title != "", title, "SAFE ZONE");
        string zone = TraderXSafeZoneTernary<string>.If(zoneName != "", zoneName, "");

        statusText = TraderXSafeZoneTernary<string>.If(zone != "", head + " - " + zone, head);
        MarkForCheck();

        m_InZone = true;
        ApplyVisibility();
    }

    void Hide()
    {
        m_InZone = false;
        if (m_Root)
            m_Root.Show(false);
    }

    // Visible only when in a zone AND no menu is up (avoids the top-center collision).
    void ApplyVisibility()
    {
        if (!m_Root)
            return;
        m_Root.Show(m_InZone && !IsMenuOpen());
    }

    bool IsMenuOpen()
    {
        return GetGame() && GetGame().GetUIManager() && GetGame().GetUIManager().GetMenu() != null;
    }
}
