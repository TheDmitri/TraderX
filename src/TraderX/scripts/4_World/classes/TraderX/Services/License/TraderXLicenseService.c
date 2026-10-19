class TraderXLicenseService
{
    static ref TraderXLicenseService m_instance;

    // Client instance: licenses owned by the local player.
    ref TraderXPlayerLicenses playerLicenses;

    // Server instance: licenses per connected player, keyed by plain (steam) id.
    ref map<string, ref TraderXPlayerLicenses> m_ServerLicenses;

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
            m_ServerLicenses = new map<string, ref TraderXPlayerLicenses>();
            TraderXModule.Event_OnTraderXPlayerJoined.Insert(OnPlayerJoined);
        }
    }

    void OnPlayerJoined(PlayerBase player, PlayerIdentity identity)
    {
        LoadPlayerLicenses(player);
    }

    // Category gating: licensesRequired holds license names (or ids, in configs from TraderX 1.x).
    bool HasLicenses(array<string> licenseNames)
    {
        if(!licenseNames || licenseNames.Count() == 0)
            return true;

        // No licenses received yet (or none owned) -> treat gated categories as locked.
        if(!playerLicenses)
            return false;

        foreach(string licenseName : licenseNames)
        {
            if (!playerLicenses.HasRequiredLicense(licenseName))
                return false;
        }

        return true;
    }

    void LoadPlayerLicenses(PlayerBase player)
    {
        if(!player || !player.GetIdentity())
            return;

        string playerId = player.GetIdentity().GetPlainId();
        TraderXPlayerLicenses loaded = TraderXLicenseRepository.Load(playerId, player.GetIdentity().GetName());
        m_ServerLicenses.Set(playerId, loaded);
        SendPlayerLicenses(player, loaded);
    }

    void SendPlayerLicenses(PlayerBase player, TraderXPlayerLicenses licenses)
    {
        if(!player || !player.GetIdentity())
            return;

        GetRPCManager().SendRPC("TraderX", "GetPlayerLicensesResponse", new Param1<TraderXPlayerLicenses>(licenses), true, player.GetIdentity());
    }

    // Server: does the player own ALL the given license names? (empty list -> yes).
    bool HasLicensesForPlayer(string playerId, array<string> licenseNames)
    {
        if(!licenseNames || licenseNames.Count() == 0)
            return true;

        TraderXPlayerLicenses licenses = GetServerPlayerLicenses(playerId);
        if(!licenses)
            return false;

        foreach(string licenseName : licenseNames)
        {
            if(!licenses.HasRequiredLicense(licenseName))
                return false;
        }

        return true;
    }

    // Server: licenses owned by a currently connected player (null if unknown).
    TraderXPlayerLicenses GetServerPlayerLicenses(string playerId)
    {
        if(m_ServerLicenses && m_ServerLicenses.Contains(playerId))
            return m_ServerLicenses.Get(playerId);

        return null;
    }

    // Server: grant a license, persist it, and resync the owning client.
    // Returns false if the player already owns it.
    bool GrantLicense(PlayerBase player, TraderXLicense license)
    {
        if(!player || !player.GetIdentity() || !license)
            return false;

        string playerId = player.GetIdentity().GetPlainId();
        TraderXPlayerLicenses licenses = GetServerPlayerLicenses(playerId);
        if(!licenses)
        {
            licenses = TraderXLicenseRepository.Load(playerId, player.GetIdentity().GetName());
            m_ServerLicenses.Set(playerId, licenses);
        }

        if(licenses.HasLicenseByName(license.licenseName))
            return false;

        licenses.AddLicense(license);
        TraderXLicenseRepository.Save(licenses);
        SendPlayerLicenses(player, licenses);
        return true;
    }

    // Client: store the licenses received from the server (was a no-op self-assign).
    void OnPlayerLicensesResponse(TraderXPlayerLicenses licenses)
    {
        playerLicenses = licenses;
    }
}
