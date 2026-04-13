static ref TXCarLockConfigService g_TXCarLockConfigService;

TXCarLockConfigService GetTXCarLockConfigService()
{
    if (!g_TXCarLockConfigService)
        g_TXCarLockConfigService = TXCarLockConfigService.Cast(DZlrServiceRegistry.GetInstance().GetService(TXCarLockConfigService));
    return g_TXCarLockConfigService;
}

class TXCarLockConfigService : DayZularService
{
    private ref TXCarLockSettings m_Settings;

    void TXCarLockConfigService()
    {
        g_TXCarLockConfigService = this;

        GetDayZularEvents().OnMissionStart.Insert(OnMissionStart);
    }

    void OnMissionStart()
    {
        if (GetGame().IsServer())
        {
            m_Settings = TXCarLockSettings.Load();
            SetCarLockClassNames();
            GetTXCarLockLogger().LogInfo("TXCarLockConfigService: Settings loaded on server");
        }
        else
        {
            m_Settings = new TXCarLockSettings();
            GetTXCarLockLogger().LogInfo("TXCarLockConfigService: Initialized on client");
        }
    }

    TXCarLockSettings GetSettings()
    {
        return m_Settings;
    }

    void SetSettings(TXCarLockSettings settings)
    {
        m_Settings = settings;
        SetCarLockClassNames();
    }

    void SetCarLockClassNames()
    {
        if (!m_Settings)
            return;

        string defaultLockpick = "CarLockPick";
        if (m_Settings.lockpickingSettings && m_Settings.lockpickingSettings.carConfig)
            defaultLockpick = m_Settings.lockpickingSettings.carConfig.lockpickClassName;
        ItemBase.SetTXCarLockClassNames(m_Settings.carLockClassName, defaultLockpick);
    }

    bool IsCarLockAdmin(int id)
    {
        if (!m_Settings)
            return false;

        foreach (string steamId : m_Settings.admins)
        {
            if (steamId.Contains(id.ToString()))
            {
                return true;
            }
        }

        return false;
    }
}
