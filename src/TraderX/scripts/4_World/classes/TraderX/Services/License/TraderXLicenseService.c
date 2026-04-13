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

    /**
     * Get the names of missing licenses from a required list.
     * Returns a comma-separated string of missing license names for UI display.
     */
    string GetMissingLicenseNames(array<string> licensesId)
    {
        if (!licensesId || licensesId.Count() == 0)
            return "";

        string missing = "";
        foreach (string licenseId : licensesId)
        {
            if (!playerLicenses || !playerLicenses.HasLicense(licenseId))
            {
                // Look up the license name from general settings
                string licenseName = GetLicenseNameById(licenseId);
                if (missing != "")
                    missing += ", ";
                missing += licenseName;
            }
        }

        return missing;
    }

    /**
     * Look up a license display name by its ID.
     */
    private string GetLicenseNameById(string licenseId)
    {
        array<ref TraderXLicense> licenses = GetTraderXModule().GetSettings().licenses;
        if (!licenses)
            return licenseId;

        foreach (TraderXLicense license : licenses)
        {
            if (license.licenseId == licenseId)
                return license.licenseName;
        }

        return licenseId;
    }

    void LoadPlayerLicenses(PlayerBase player)
    {
        TraderXPlayerLicenses playerLicenses = TraderXLicenseRepository.Load(player.GetIdentity().GetPlainId(), player.GetIdentity().GetName());
        GetDayZularRPCManager().SendRPC("TraderX", "GetPlayerLicensesResponse", new Param1<TraderXPlayerLicenses>(playerLicenses), true, player.GetIdentity());
    }

    void OnPlayerLicensesResponse(TraderXPlayerLicenses playerLicenses)
    {
        playerLicenses = playerLicenses;
    }
}