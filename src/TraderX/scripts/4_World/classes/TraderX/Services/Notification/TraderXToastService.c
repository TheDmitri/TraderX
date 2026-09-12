/**
 * TraderXToastService — client-side FIFO runner for the HUD toast card.
 *
 * Port of CourseBarterTrader's BarterNotificationService. Transaction events
 * (purchase, sale, partial, error) are pushed in via ShowToast()/ShowNotification();
 * the runner queues them FIFO and shows ONE themed card at a time on the HUD:
 *   ENTER (slide-in) -> HOLD (progress drains) -> EXIT (slide-out) -> GAP -> next.
 * A "+N QUEUED" pill shows while more wait.
 *
 * Replaces the legacy multi-card NotificationContainer/NotificationCard MVVM
 * stack (kept dormant until this is validated in-game). The card is a plain
 * DZlrComponentBase mounted on the workspace (HUD overlay — no cursor, no pause).
 *
 * DayZular service: registered in TraderXAppInit; resolved via GetTraderXToastService().
 */
static ref TraderXToastService g_TraderXToastService;

static TraderXToastService GetTraderXToastService()
{
    if (!g_TraderXToastService)
        g_TraderXToastService = TraderXToastService.Cast(DZlrServiceRegistry.GetInstance().GetService(TraderXToastService));
    return g_TraderXToastService;
}

class TraderXToastService : DayZularService
{
    private static const int PHASE_IDLE  = 0;
    private static const int PHASE_ENTER = 1;
    private static const int PHASE_HOLD  = 2;
    private static const int PHASE_EXIT  = 3;
    private static const int PHASE_GAP   = 4;

    private static const float SLIDE_OFFSET_FRAC = 0.25;   // offscreen-right start/end (screen fraction)

    private ref array<ref TraderXToastNotification> m_Queue;
    private ref TraderXToastCardComponent m_Card;
    private int m_Phase;
    private float m_PhaseMs;
    private float m_HoldMs;   // per-toast hold; longer when there is a breakdown to read

    void TraderXToastService()
    {
        g_TraderXToastService = this;
        m_Queue = new array<ref TraderXToastNotification>();
        m_Phase = PHASE_IDLE;

        GetDayZularEvents().OnMissionStart.Insert(Initialize);
        GetDayZularEvents().OnUpdate.Insert(OnTick);
    }

    void ~TraderXToastService()
    {
        if (GetDayZularEvents())
        {
            GetDayZularEvents().OnMissionStart.Remove(Initialize);
            GetDayZularEvents().OnUpdate.Remove(OnTick);
        }
    }

    void Initialize()
    {
        if (!GetGame().IsClient())
            return;

        // Mount the card now (hidden); reused for every notification.
        m_Card = new TraderXToastCardComponent();
        m_Card.InitComponent(null);
    }

    // =========================================================================
    // Public API
    // =========================================================================

    /**
     * Rich toast: explicit type (TXNOTIF_*), title, body, optional footer.
     */
    void ShowToast(string type, string title, string body, string footer = "", array<string> details = null)
    {
        TraderXToastNotification notif = new TraderXToastNotification(type, title, body, footer, "now");
        if (details)
        {
            foreach (string line : details)
                notif.AddDetail(line);
        }
        Enqueue(notif);
    }

    /**
     * Back-compat entry: a flat message + success flag. Maps to a generic INFO
     * card (success) or ERROR card (failure).
     */
    void ShowNotification(string message, bool isSuccess = true)
    {
        if (message == "")
            return;

        string type = DZTernary<string>.If(isSuccess, TXNOTIF_INFO, TXNOTIF_ERROR);
        string title = DZTernary<string>.If(isSuccess, "Trader", "Action Failed");
        ShowToast(type, title, message);
    }

    private void Enqueue(TraderXToastNotification notif)
    {
        if (!notif || !notif.IsValid())
            return;

        // Console trail.
        Print("[TraderX] " + notif.GetKicker() + " — " + notif.title + ": " + notif.body);

        if (!GetGame().IsClient())
            return;

        m_Queue.Insert(notif);
    }

    int PendingCount()
    {
        return m_Queue.Count();
    }

    // =========================================================================
    // HUD card state machine (client tick; deltaTime in seconds)
    // =========================================================================

    void OnTick(float deltaTime)
    {
        if (!GetGame().IsClient() || !m_Card)
            return;

        float dtMs = deltaTime * 1000.0;
        m_PhaseMs += dtMs;

        switch (m_Phase)
        {
            case PHASE_IDLE:
                if (m_Queue.Count() > 0)
                    BeginNext();
                break;

            case PHASE_ENTER:
            {
                float et = Math.Clamp(m_PhaseMs / TX_TOAST_IN_MS, 0.0, 1.0);
                m_Card.SetSlideOffset(SLIDE_OFFSET_FRAC * (1.0 - et));
                if (m_PhaseMs >= TX_TOAST_IN_MS)
                {
                    m_Card.SetSlideOffset(0);
                    GoToPhase(PHASE_HOLD);
                    m_Card.StartHold(m_HoldMs);
                }
                break;
            }

            case PHASE_HOLD:
            {
                m_Card.Tick(dtMs);
                if (m_PhaseMs >= m_HoldMs)
                    GoToPhase(PHASE_EXIT);
                break;
            }

            case PHASE_EXIT:
            {
                float xt = Math.Clamp(m_PhaseMs / TX_TOAST_OUT_MS, 0.0, 1.0);
                m_Card.SetSlideOffset(SLIDE_OFFSET_FRAC * xt);
                if (m_PhaseMs >= TX_TOAST_OUT_MS)
                {
                    m_Card.ShowCard(false);
                    GoToPhase(PHASE_GAP);
                }
                break;
            }

            case PHASE_GAP:
                if (m_PhaseMs >= TX_TOAST_GAP_MS)
                    GoToPhase(PHASE_IDLE);
                break;
        }
    }

    private void GoToPhase(int phase)
    {
        m_Phase = phase;
        m_PhaseMs = 0;
    }

    private void BeginNext()
    {
        TraderXToastNotification notif = m_Queue.Get(0);
        m_Queue.RemoveOrdered(0);

        // Give bulk toasts extra dwell time — one breakdown line is one more thing to read.
        m_HoldMs = TX_TOAST_HOLD_MS;
        if (notif.HasDetails())
            m_HoldMs += notif.details.Count() * 700.0;

        m_Card.Apply(notif);
        m_Card.SetQueueCount(m_Queue.Count());    // backlog still waiting
        m_Card.SetSlideOffset(SLIDE_OFFSET_FRAC);  // start offscreen
        m_Card.ShowCard(true);
        GoToPhase(PHASE_ENTER);
    }
}
