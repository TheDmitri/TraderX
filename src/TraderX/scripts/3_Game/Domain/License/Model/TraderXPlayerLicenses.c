class TraderXPlayerLicenses
{
    string steamId;
    string playerName;

    ref array<ref TraderXLicense> licenses;

    void TraderXPlayerLicenses(string steamId, string playerName)
    {
        this.steamId = steamId;
        this.playerName = playerName;

        this.licenses = new array<ref TraderXLicense>();
    }

    void AddLicense(TraderXLicense license)
    {
        licenses.Insert(license);
    }

    void RemoveLicense(TraderXLicense license)
    {
       for(int i = licenses.Count() - 1; i >= 0; i--)
       {
           if(licenses[i].licenseId == license.licenseId)
           {
               licenses.Remove(i);
               break;
           }
       }
    }

    bool HasLicense(string licenseId)
    {
        for(int i = 0; i < licenses.Count(); i++)
        {
            if(licenses[i].licenseId == licenseId)
                return true;
        }
        return false;
    }

    // Category gating stores license NAMES (TraderXCategory.licensesRequired), so the
    // ownership check must match by name, not by the generated UUID.
    // licensesRequired entries are license names, but configs written by TraderX 1.x hold
    // license ids: accept either so existing gated categories and traders keep working.
    bool HasRequiredLicense(string nameOrId)
    {
        if (HasLicenseByName(nameOrId))
            return true;
        return HasLicense(nameOrId);
    }

    bool HasLicenseByName(string licenseName)
    {
        for(int i = 0; i < licenses.Count(); i++)
        {
            if(CF_String.EqualsIgnoreCase(licenses[i].licenseName, licenseName))
                return true;
        }
        return false;
    }
}

