class PanelShadowView extends ScriptedWidgetEventHandler
{
    protected Widget m_Root;
    protected Widget m_ContentPanel;
    
    void PanelShadowView(Widget parent)
    {
        m_Root = GetGame().GetWorkspace().CreateWidgets("TraderX/datasets/gui/TraderXPrefab/Panel/PanelShadowView.layout", parent);
        m_ContentPanel = m_Root.FindAnyWidget("ContentPanel");
    }
    
    void ~PanelShadowView()
    {
        if (m_Root)
            m_Root.Unlink();
    }
    
    Widget GetRoot() { return m_Root; }
    Widget GetContentPanel() { return m_ContentPanel; }
    
    void SetSize(float width, float height)
    {
        if (m_Root)
            m_Root.SetSize(width, height);
    }
    
    void SetPosition(float x, float y)
    {
        if (m_Root)
            m_Root.SetPos(x, y);
    }
    
    void SetColor(int color)
    {
        if (m_Root)
            m_Root.SetColor(color);
    }
    
    void Show(bool show)
    {
        if (m_Root)
            m_Root.Show(show);
    }
}
