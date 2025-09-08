class TraderXLicenseService
{
    static ref TraderXLicenseService m_instance;

    //Client instance
    ref TraderXPlayerLicenses playerLicenses;

    static TraderXLicenseService GetInstance()
    {
        if (!m_instance)
        {
            m_instance = new TraderXLicenseService();
        }
        return m_instance;
    }

    void TraderXLicenseService()
    {
        if(GetGame().IsServer())
        {
            TraderXModule.Event_OnTraderXPlayerJoined.Insert(OnPlayerJoined);
        }
    }

    void OnPlayerJoined(PlayerBase player, PlayerIdentity identity)
    {
        LoadPlayerLicenses(player);
    }

    bool HasLicenses(array<string> licensesId)
    {
        if(licensesId.Count() == 0)
            return true;

        foreach(string licenseId : licensesId)
        {
            if (!playerLicenses.HasLicense(licenseId))
                return false;
        }

        return true;
    }

    void LoadPlayerLicenses(PlayerBase player)
    {
        TraderXPlayerLicenses playerLicenses = TraderXLicenseRepository.Load(player.GetIdentity().GetPlainId(), player.GetIdentity().GetName());
        GetRPCManager().SendRPC("TraderX", "GetPlayerLicensesResponse", new Param1<TraderXPlayerLicenses>(playerLicenses), true, player.GetIdentity());
    }

    void OnPlayerLicensesResponse(TraderXPlayerLicenses playerLicenses)
    {
        playerLicenses = playerLicenses;
    }
}