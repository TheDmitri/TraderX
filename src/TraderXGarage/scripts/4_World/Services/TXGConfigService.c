// TraderXGarage - Config Service
// Server: loads TXGSettings from disk.
// Client: holds a lightweight cache (costs, slots, npc class names) synced on join.

class TXGConfigService extends DayZularService
{
    protected ref TXGSettings m_ServerSettings;

    // Client cache (populated by RPC on join)
    protected int  m_ClientParkInCost;
    protected int  m_ClientParkOutCost;
    protected int  m_ClientMaxSlots = 5;
    protected bool m_ClientEnableRepairs = true;
    protected bool m_ClientEnableInsurance = false;
    protected int  m_ClientInsuranceDefaultDays = 1;
    protected bool m_ClientIsAdmin = false;
    protected int  m_ClientInsuranceMaxDays = 30;
    protected bool m_ClientReceiptsEnabled = false;
    protected ref TStringArray m_ClientNpcClasses = new TStringArray();
    protected ref array<int> m_ClientNpcIds = new array<int>();
    protected ref array<ref TXGNpcSpotDTO> m_ClientNpcSpots = new array<ref TXGNpcSpotDTO>();

    void TXGConfigService()
    {
        if (GetGame().IsServer() || !GetGame().IsMultiplayer())
        {
            // Migrate a TraderPlus V1 garage config first (no-op if none / already configured).
            oldTraderPlusGarageConfig.ConvertIfPresent();
            m_ServerSettings = TXGSettings.Load();
        }
    }

    protected bool m_CurrenciesSyncedFromCore = false;

    TXGSettings GetServerSettings()
    {
#ifdef TraderX
        if (m_ServerSettings && !m_CurrenciesSyncedFromCore)
        {
            SyncCurrenciesFromTraderX();
        }
#endif
        return m_ServerSettings;
    }

#ifdef TraderX
    protected void SyncCurrenciesFromTraderX()
    {
        TraderXCurrencyService curSvc = TraderXCurrencyService.m_instance;
        if (!curSvc || !curSvc.currencySettings || !curSvc.currencySettings.currencyTypes)
            return;
        m_CurrenciesSyncedFromCore = true;
        foreach (TraderXCurrencyType curType : curSvc.currencySettings.currencyTypes)
        {
            if (!curType || !curType.currencies)
                continue;
            foreach (TraderXCurrency cur : curType.currencies)
            {
                if (!cur || cur.className == "" || cur.value <= 0)
                    continue;
                if (!m_ServerSettings.IsCurrency(cur.className))
                {
                    m_ServerSettings.currencies.Insert(new TXGCurrencyEntry(cur.className, cur.value));
                }
            }
        }
    }
#endif

    // --- Client cache accessors ---

    int  GetClientParkInCost()           { return m_ClientParkInCost; }
    int  GetClientParkOutCost()          { return m_ClientParkOutCost; }
    int  GetClientMaxSlots()             { return m_ClientMaxSlots; }
    bool GetClientEnableRepairs()        { return m_ClientEnableRepairs; }
    bool GetClientEnableInsurance()      { return m_ClientEnableInsurance; }
    int  GetClientInsuranceDefaultDays() { return m_ClientInsuranceDefaultDays; }
    bool GetClientIsAdmin()              { return m_ClientIsAdmin; }
    int  GetClientInsuranceMaxDays()     { return m_ClientInsuranceMaxDays; }
    bool GetClientReceiptsEnabled()      { return m_ClientReceiptsEnabled; }

    void SetClientConfig(int parkIn, int parkOut, int maxSlots, array<string> npcClasses, array<int> npcIds = null, array<ref TXGNpcSpotDTO> npcSpots = null, bool enableRepairs = true, bool enableInsurance = false, int insDefaultDays = 1, bool isAdmin = false, int insMaxDays = 30, bool receiptsEnabled = false)
    {
        m_ClientInsuranceMaxDays = insMaxDays;
        m_ClientReceiptsEnabled = receiptsEnabled;
        m_ClientParkInCost = parkIn;
        m_ClientParkOutCost = parkOut;
        m_ClientMaxSlots = maxSlots;
        m_ClientEnableRepairs = enableRepairs;
        m_ClientEnableInsurance = enableInsurance;
        m_ClientInsuranceDefaultDays = insDefaultDays;
        m_ClientIsAdmin = isAdmin;

        m_ClientNpcClasses.Clear();
        if (npcClasses)
        {
            foreach (string c : npcClasses)
            {
                if (c != "" && m_ClientNpcClasses.Find(c) == -1)
                    m_ClientNpcClasses.Insert(c);
            }
        }

        m_ClientNpcIds.Clear();
        if (npcIds)
        {
            foreach (int id : npcIds)
            {
                if (id >= 0 && m_ClientNpcIds.Find(id) == -1)
                    m_ClientNpcIds.Insert(id);
            }
        }

        m_ClientNpcSpots.Clear();
        if (npcSpots)
        {
            foreach (TXGNpcSpotDTO s : npcSpots)
            {
                if (s)
                    m_ClientNpcSpots.Insert(s);
            }
        }
    }

    bool IsGarageNpcId(int npcId)
    {
        if (npcId < 0)
            return false;
        if (npcId == TXGConstants.GARAGE_NPC_ID)
            return true;
        return m_ClientNpcIds.Find(npcId) != -1;
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

    bool ResolveClientParkSpot(int npcId, vector playerPos, out vector outPos, out vector outOri)
    {
        vector size;
        return ResolveClientParkSpotEx(npcId, playerPos, outPos, outOri, size);
    }

    bool ResolveClientParkSpotEx(int npcId, vector playerPos, out vector outPos, out vector outOri, out vector outSize)
    {
        outSize = vector.Zero;
        outPos = vector.Zero;
        outOri = vector.Zero;
        if (!m_ClientNpcSpots || m_ClientNpcSpots.Count() == 0)
            return false;

        int targetNpcId = npcId;
        if (targetNpcId < 0)
        {
            float bestDist = -1;
            foreach (TXGNpcSpotDTO candidate : m_ClientNpcSpots)
            {
                if (!candidate)
                    continue;
                float d = vector.Distance(playerPos, candidate.npcPos);
                if (bestDist < 0 || d < bestDist)
                {
                    bestDist = d;
                    targetNpcId = candidate.npcId;
                }
            }
        }

        foreach (TXGNpcSpotDTO spot : m_ClientNpcSpots)
        {
            if (!spot || spot.npcId != targetNpcId)
                continue;
            outPos = spot.spotPos;
            outOri = spot.spotOri;
            outSize = spot.spotSize;
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
