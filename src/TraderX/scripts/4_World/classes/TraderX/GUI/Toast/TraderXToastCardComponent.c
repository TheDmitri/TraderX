/**
 * TraderXToastCardComponent — the in-game HUD toast card for the marketplace.
 *
 * Top-right HUD card themed with the TraderX Field-Ledger 9-slice styles and the gold/
 * bronze accent palette. Renders ONE TraderXToastNotification and animates its own
 * draining progress rule; the FIFO queue + slide timing live in the client runner
 * (TraderXToastService).
 *
 * GROW-TO-CONTENT (ported from PDANotifications): cardSlot is a GridSpacer with
 * "Size To Content V" and every text row carries size-to-text-v, so a long title / body /
 * per-transaction breakdown grows its row and the grid grows the card. Nothing is clipped
 * and no height is computed in code. The layout is authored in % so the card keeps its
 * proportions and stays pinned to the right edge at 1080p / 1440p / 4K.
 *
 * leftRule and queuePill are overlay SIBLINGS (a grid turns every child into a row), trailed
 * to the card's live geometry by SyncOverlays().
 *
 * Widget-name contract (must match data-name in TraderXToastCard.html):
 *   cardSlot, leftRule, iconImg, detailsText, footerText, progressTrack, progressFill, queuePill
 * Bindings: the card binds the notification object directly —
 *   {{ _notif.GetKicker() }}, {{ _notif.title }}, {{ _notif.body }},
 *   {{ _notif.footer }}, {{ _notif.time }} — plus {{ queue_count }} for the
 *   backlog pill. The icon frame is loaded in code (ApplyTypeIcon).
 */
class TraderXToastCardComponent extends DZlrComponentBase
{
    private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("traderx-toast-card", TraderXToastCardComponent);

    // The notification being rendered (its fields/methods are bound by the layout).
    ref TraderXToastNotification _notif;

    // Backlog count ({{ queue_count }}) — not part of the notification itself.
    string queue_count;

    // --- Widgets bound by name (AutoLoadWidgets) ---
    Widget cardSlot;
    Widget leftRule;
    ImageWidget iconImg;
    Widget progressTrack;
    Widget progressFill;
    Widget queuePill;
    Widget detailsText;
    Widget footerText;

    // Gap below the card for the queue pill, in px (converted to a screen fraction at runtime).
    private static const float PILL_GAP_PX = 6.0;

    // Hold timing for the progress rule.
    private float m_HoldMs;
    private float m_ElapsedMs;
    private bool m_Running;

    // cardSlot base position (px), captured at init, for slide animation.
    private float m_SlotBaseX;
    private float m_SlotBaseY;

    override protected void RegisterMetadata()
    {
        m_Metadata = new ComponentMetadata();
        m_Metadata.selector = "traderx-toast-card";
        m_Metadata.templatePath = "TraderX/datasets/gui/Notification/TraderXToastCard.layout";
    }

    override void OnInit()
    {
        super.OnInit();

        // cardSlot is right-anchored in the layout (halign right_ref), so m_SlotBaseX is an inset
        // from the screen's RIGHT edge — resolution independent (1080p, 4K, any UI scale).
        if (cardSlot)
            cardSlot.GetPos(m_SlotBaseX, m_SlotBaseY);

        SetQueueCount(0);
        ShowCard(false);
        MarkForCheck();
    }

    // Toggle the whole card on the HUD.
    void ShowCard(bool show)
    {
        if (m_Root)
            m_Root.Show(show);
    }

    // Slide the card horizontally by a RELATIVE fraction of the screen from its base position
    // (offscreen-right at ~0.25). Relative units because the layout is authored in % — px would
    // not match. Driven by the runner's enter/exit animation.
    void SetSlideOffset(float dxFraction)
    {
        if (cardSlot)
            cardSlot.SetPos(m_SlotBaseX + dxFraction, m_SlotBaseY);
        SyncOverlays();
    }

    // Trail the overlay siblings onto the card's LIVE geometry. cardSlot is a grid with
    // Size-To-Content-V, so its height changes per notification (grow-to-content) and the
    // accent rule / backlog pill can't be static % siblings. Everything is worked in SCREEN
    // FRACTIONS: read the card's live screen rect and convert to fractions of the screen.
    //  - leftRule follows the card's left edge AND height (so it also slides with the card).
    //  - queuePill keeps its authored X (it must NOT slide off with the card); only Y is driven.
    void SyncOverlays()
    {
        if (!cardSlot || !m_Root)
            return;

        float scrW, scrH;
        m_Root.GetScreenSize(scrW, scrH);
        if (scrW <= 0 || scrH <= 0)
            return;

        float sx, sy, sw, sh;
        cardSlot.GetScreenPos(sx, sy);
        cardSlot.GetScreenSize(sw, sh);
        if (sw <= 0 || sh <= 0)
            return;

        if (leftRule)
        {
            float ruleW, ruleH;
            leftRule.GetSize(ruleW, ruleH);
            leftRule.SetPos(sx / scrW, sy / scrH);
            leftRule.SetSize(ruleW, sh / scrH);
        }

        if (queuePill)
        {
            float px, py;
            queuePill.GetPos(px, py);
            queuePill.SetPos(px, (sy + sh + PILL_GAP_PX) / scrH);
        }
    }

    // Render a notification onto the card: load the type's atlas frame + recolor
    // the accent widgets by type.
    void Apply(TraderXToastNotification notif)
    {
        if (!notif)
            return;

        _notif = notif;
        ApplyTypeIcon(notif.GetIconImageRef());
        ApplyAccent(notif);
        ApplyDetailsLayout();
        MarkForCheck();
        SyncOverlays();
    }

    // Bulk trades carry a per-transaction breakdown. Only the VISIBILITY is decided here —
    // the details row carries size-to-text-v and the card grid carries Size-To-Content-V, so
    // the row grows to fit every line and the card grows with it. No clipping, no line-count
    // math (the old fixed 18px-per-line estimate is what used to cut the breakdown off).
    void ApplyDetailsLayout()
    {
        bool expanded = _notif && _notif.HasDetails();

        if (detailsText) detailsText.Show(expanded);
        if (footerText) footerText.Show(!expanded);
    }

    // Load the type's frame from the traderx_icons atlas. The widget's accent
    // color tints the white glyph (set in ApplyAccent).
    private void ApplyTypeIcon(string imageRef)
    {
        if (!iconImg)
            return;
        iconImg.LoadImageFile(0, imageRef);
        iconImg.SetImage(0);
    }

    // Left rule / glyph / progress take the type accent (gold, green, amber, red).
    private void ApplyAccent(TraderXToastNotification notif)
    {
        int accent = notif.GetAccentColor();
        int track = notif.GetAccentTrackColor();

        if (leftRule) leftRule.SetColor(accent);
        if (iconImg) iconImg.SetColor(accent);
        if (progressFill) progressFill.SetColor(accent);
        if (progressTrack) progressTrack.SetColor(track);
    }

    // --- Progress rule: drain width 1 -> 0 across the hold. ---
    void StartHold(float holdMs)
    {
        m_HoldMs = holdMs;
        m_ElapsedMs = 0;
        m_Running = true;
        SetProgress(1.0);
    }

    void StopHold()
    {
        m_Running = false;
    }

    // Called each frame by the runner with the frame delta (ms).
    void Tick(float deltaMs)
    {
        if (!m_Running)
            return;

        m_ElapsedMs += deltaMs;
        float ratio = 1.0 - (m_ElapsedMs / m_HoldMs);
        if (ratio < 0)
            ratio = 0;

        SetProgress(ratio);
        SyncOverlays();   // card is settled by now — keep the rule/pill trailing its height

        if (m_ElapsedMs >= m_HoldMs)
            m_Running = false;
    }

    // ratio in [0,1] -> progress fill width fraction of the track.
    void SetProgress(float ratio)
    {
        if (!progressFill)
            return;
        float w, h;
        progressFill.GetSize(w, h);
        progressFill.SetSize(ratio, h);
    }

    void SetQueueCount(int count)
    {
        if (count > 0)
            queue_count = "+" + count.ToString();
        else
            queue_count = string.Empty;

        if (queuePill)
            queuePill.Show(count > 0);

        MarkForCheck();
    }
}
