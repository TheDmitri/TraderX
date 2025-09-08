class TraderXUINavigationService
{
    static ref TraderXUINavigationService m_instance;

    private int navigationId;

    static TraderXUINavigationService GetInstance()
    {
        if (!m_instance)
        {
            m_instance = new TraderXUINavigationService();
        }
        return m_instance;
    }

    void SetNavigationId(int id)
    {
        navigationId = id;
    }

    int GetNavigationId()
    {
        return navigationId;
    }
}