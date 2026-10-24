class TraderXLicenseService
{
    static ref TraderXLicenseService m_instance;

    // Client: fired when the local player's licenses arrive from the server (cards re-apply their locked state).
    static ref ScriptInvoker Event_OnPlayerLicensesChanged = new ScriptInvoker();

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
        Event_OnPlayerLicensesChanged.Invoke();
    }

    // Client: the category of the OPEN trader that keeps this product licence-locked, or null.
    // A product is locked only when every trader category listing it requires a licence the
    // player lacks (same rule as the server's TraderXTransactionValidator category check).
    // Variants/attachments not listed directly in a category are never reported locked here;
    // callers check their parent product instead.
    TraderXCategory GetLockingCategory(string productId)
    {
        if (productId == string.Empty)
            return null;

        TraderXTradingService trading = TraderXTradingService.GetInstance();
        if (!trading || !trading.traderNpc || !trading.traderNpc.categoriesId)
            return null;

        TraderXCategory locking = null;
        foreach (string categoryId : trading.traderNpc.categoriesId)
        {
            TraderXCategory category = TraderXCategoryRepository.GetCategoryById(categoryId);
            if (!category || !category.ContainsProduct(productId))
                continue;
            if (HasLicenses(category.licensesRequired))
                return null;
            if (!locking)
                locking = category;
        }
        return locking;
    }

    bool IsProductLicenseLocked(TraderXProduct product)
    {
        if (!product)
            return false;
        return GetLockingCategory(product.GetProductId()) != null;
    }

    // "Car Licence, Boat Licence" — licensesRequired may hold 1.x ids, resolve them to names.
    static string JoinLicenseNames(array<string> licenseNames)
    {
        string result = "";
        if (!licenseNames)
            return result;

        foreach (string nameOrId : licenseNames)
        {
            if (result != "")
                result = result + ", ";
            result = result + GetLicenseDisplayName(nameOrId);
        }
        return result;
    }

    static string GetLicenseDisplayName(string nameOrId)
    {
        if (!GetTraderXModule() || !GetTraderXModule().GetSettings() || !GetTraderXModule().GetSettings().licenses)
            return nameOrId;

        foreach (TraderXLicense license : GetTraderXModule().GetSettings().licenses)
        {
            if (license && license.licenseId == nameOrId)
                return license.licenseName;
        }
        return nameOrId;
    }

    // Client: "Licenses Required: Car Licence" (localized prefix) for card badges and toasts.
    static string FormatRequiredLabel(array<string> licenseNames)
    {
        return Widget.TranslateString("#tpm_licenses_required") + ": " + JoinLicenseNames(licenseNames);
    }

    // Client: block-with-reason feedback for a buy attempt on a licence-locked product.
    // Returns true (and shows the toast) when the product is locked.
    bool NotifyIfProductLocked(TraderXProduct product)
    {
        if (!product)
            return false;

        TraderXCategory locking = GetLockingCategory(product.GetProductId());
        if (!locking)
            return false;

        TraderXInventoryManager.PlayMenuSound(ETraderXSounds.ERROR);
        if (GetTraderXToastService())
            GetTraderXToastService().ShowToast(TXNOTIF_ERROR, "#tpm_licenses_required", product.GetDisplayName() + " - " + JoinLicenseNames(locking.licensesRequired));
        return true;
    }
}
