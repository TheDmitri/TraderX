/**
 * TraderXToastCardComponent — the in-game HUD toast card for the marketplace.
 *
 * Port of CourseBarterTrader's BarterToastCardComponent: a single 360x152 card,
 * top-right, themed with the TraderX Field-Ledger 9-slice styles and the gold/
 * bronze accent palette. Renders ONE TraderXToastNotification and animates its
 * own draining progress rule; the FIFO queue + slide timing live in the client
 * runner (TraderXToastService).
 *
 * Widget-name contract (must match data-name in TraderXToastCard.html):
 *   cardSlot, leftRule, iconImg, progressTrack, progressFill, queuePill
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
    Widget detailsClip;
    Widget footerText;

    // px geometry for the auto-expanding per-transaction breakdown.
    private static const float TX_CARD_W       = 360.0;
    private static const float TX_CARD_BASE_H  = 152.0;
    private static const float TX_DET_TOP_Y    = 110.0;  // detailsClip top inside the card
    private static const float TX_DET_LINE_H   = 18.0;
    private static const float TX_DET_PAD      = 16.0;   // top + bottom padding inside the panel
    private static const float TX_DET_GAP      = 6.0;    // gap below details before the progress rule
    private static const float TX_DET_W        = 332.0;
    private static const float TX_PROGRESS_H   = 3.0;

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

    // Slide the card horizontally by dx px from its base position (offscreen-right
    // when dx is large). Driven by the runner's enter/exit animation.
    void SetSlideOffsetPx(float dx)
    {
        if (cardSlot)
            cardSlot.SetPos(m_SlotBaseX + dx, m_SlotBaseY);
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
    }

    // Bulk trades carry a per-transaction breakdown; auto-expand the card to show it
    // (the HUD toast has no cursor, so there is no click-to-expand toggle). Single
    // toasts collapse back to the base 152px card. Card is reused, so reset both ways.
    void ApplyDetailsLayout()
    {
        bool expanded = _notif && _notif.HasDetails();
        float cardH = TX_CARD_BASE_H;

        if (expanded)
        {
            float detailsH = TX_DET_PAD + _notif.details.Count() * TX_DET_LINE_H;
            cardH = TX_DET_TOP_Y + detailsH + TX_DET_GAP + TX_PROGRESS_H;

            if (detailsClip)
            {
                detailsClip.SetSize(TX_DET_W, detailsH);
                detailsClip.Show(true);
            }
            if (footerText) footerText.Show(false);
        }
        else
        {
            if (detailsClip) detailsClip.Show(false);
            if (footerText) footerText.Show(true);
        }

        if (cardSlot) cardSlot.SetSize(TX_CARD_W, cardH);

        if (leftRule)
        {
            float lw, lh;
            leftRule.GetSize(lw, lh);
            leftRule.SetSize(lw, cardH);
        }

        float py = cardH - TX_PROGRESS_H;
        if (progressTrack) progressTrack.SetPos(0, py);
        if (progressFill) progressFill.SetPos(0, py);
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
