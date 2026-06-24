// TraderXGarage - Config Service
// Server: loads TXGSettings from disk.
// Client: holds a lightweight cache (costs, slots, npc class names) synced on join.

class TXGConfigService extends DayZularService
{
    protected ref TXGSettings m_ServerSettings;

    // Client cache (populated by RPC on join)
    protected int m_ClientParkInCost;
    protected int m_ClientParkOutCost;
    protected int m_ClientMaxSlots = 5;
    protected ref TStringArray m_ClientNpcClasses = new TStringArray();

    void TXGConfigService()
    {
        if (GetGame().IsServer() || !GetGame().IsMultiplayer())
        {
            m_ServerSettings = TXGSettings.Load();
        }
    }

    TXGSettings GetServerSettings()
    {
        return m_ServerSettings;
    }

    // --- Client cache accessors ---

    int GetClientParkInCost()  { return m_ClientParkInCost; }
    int GetClientParkOutCost() { return m_ClientParkOutCost; }
    int GetClientMaxSlots()    { return m_ClientMaxSlots; }

    void SetClientConfig(int parkIn, int parkOut, int maxSlots, array<string> npcClasses)
    {
        m_ClientParkInCost = parkIn;
        m_ClientParkOutCost = parkOut;
        m_ClientMaxSlots = maxSlots;

        m_ClientNpcClasses.Clear();
        if (npcClasses)
        {
            foreach (string c : npcClasses)
            {
                if (c != "")
                    m_ClientNpcClasses.Insert(c);
            }
        }
    }

    bool IsGarageNpcClass(string className)
    {
        foreach (string c : m_ClientNpcClasses)
        {
            if (c == className)
                return true;
        }
        return false;
    }
}

// Global accessor
static ref TXGConfigService g_TXGConfigService;

TXGConfigService GetTXGConfigService()
{
    if (!g_TXGConfigService)
        g_TXGConfigService = TXGConfigService.Cast(DZlrServiceRegistry.GetInstance().GetService(TXGConfigService));
    return g_TXGConfigService;
}
