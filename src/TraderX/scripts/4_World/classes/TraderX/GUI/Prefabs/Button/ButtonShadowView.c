class ButtonShadowView extends ScriptedWidgetEventHandler
{
    protected Widget m_Root;
    protected TextWidget m_ButtonText;
    protected ImageWidget m_ButtonIcon;
    protected string m_Command;
    
    void ButtonShadowView(Widget parent)
    {
        m_Root = GetGame().GetWorkspace().CreateWidgets("TraderX/datasets/gui/TraderXPrefab/Button/ButtonShadowView.layout", parent);
        m_ButtonText = TextWidget.Cast(m_Root.FindAnyWidget("ButtonText"));
        m_ButtonIcon = ImageWidget.Cast(m_Root.FindAnyWidget("ButtonIcon"));
        
        m_Root.SetHandler(this);
    }
    
    void ~ButtonShadowView()
    {
        if (m_Root)
            m_Root.Unlink();
    }
    
    Widget GetRoot() { return m_Root; }
    
    void SetText(string text)
    {
        if (m_ButtonText)
            m_ButtonText.SetText(text);
    }
    
    void SetIcon(string path)
    {
        if (m_ButtonIcon)
        {
            m_ButtonIcon.Show(true);
            m_ButtonIcon.LoadImageFile(0, path);
        }
    }
    
    void SetCommand(string command)
    {
        m_Command = command;
    }
    
    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (w == m_Root && button == MouseState.LEFT)
        {
            if (m_Command != "")
                GetGame().GameScript.CallFunction(this, m_Command, null, 0);
            return true;
        }
        return false;
    }
    
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
