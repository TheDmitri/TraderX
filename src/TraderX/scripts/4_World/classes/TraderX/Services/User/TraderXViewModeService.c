// Global product-list view mode (LIST rows vs GRID tiles).
// Single source of truth — any list container subscribes to the event and
// shows/hides its list/grid sub-containers. No per-page state duplication.
class TraderXViewModeService
{
    static ref TraderXViewModeService m_instance;

    // Invoked with the new mode: true = grid (tiles), false = list (rows).
    static ref ScriptInvoker Event_OnViewModeChangedCallBack = new ScriptInvoker();

    private bool m_isGridView = false;

    static TraderXViewModeService GetInstance()
    {
        if (!m_instance)
            m_instance = new TraderXViewModeService();
        return m_instance;
    }

    bool IsGridView()
    {
        return m_isGridView;
    }

    void SetGridView(bool grid)
    {
        if (m_isGridView == grid)
            return;
        m_isGridView = grid;
        Event_OnViewModeChangedCallBack.Invoke(m_isGridView);
    }

    void Toggle()
    {
        SetGridView(!m_isGridView);
    }
}
