// TraderXGarage - Insurance Policy Persistence (Server-authoritative)
// File: $profile:TraderX/TraderXGarage/Insurance/Policies_<steam64>.json
// Implements docs/handoff/INSURANCE_SPEC.md §4.1.

class TXGInsurancePolicy
{
    int    version = 1;
    string policyId;            // unique id, also stamped on the vehicle via CF ModStorage
    string ownerUid;            // steam64
    string vehicleClassName;    // lowercase
    int    coveredValue;        // catalog price at signing (server)
    int    premiumPaid;         // total paid, for the ledger/report
    int    startedAt;           // unix seconds (server clock)
    int    expiresAt;
    int    status;              // TXGConstants.POLICY_ACTIVE / EXPIRED / CLAIMED / VOID
    int    claimedAt;
    string voidReason;          // "SOLD", "DELETED", "OWNER_CHANGED", "ADMIN", "EXPIRED"

    void TXGInsurancePolicy() {}
}

class TXGPlayerPolicies
{
    int    version = 1;
    string ownerUid;
    int    lastClaimAt = 0;
    int    nextPolicyCounter = 0;
    ref array<ref TXGInsurancePolicy> policies;

    void TXGPlayerPolicies()
    {
        policies = new array<ref TXGInsurancePolicy>();
    }

    static string GetFilePath(string uid)
    {
        return TXGConstants.INSURANCE_DIR + "Policies_" + uid + ".json";
    }

    string AllocatePolicyId()
    {
        nextPolicyCounter = nextPolicyCounter + 1;
        return "POL_" + ownerUid + "_" + nextPolicyCounter.ToString();
    }

    TXGInsurancePolicy FindPolicy(string policyId)
    {
        if (policyId == "" || !policies)
            return null;
        foreach (TXGInsurancePolicy p : policies)
        {
            if (p && p.policyId == policyId)
                return p;
        }
        return null;
    }

    int RefreshExpirations(int nowUnix)
    {
        int expiredNow = 0;
        if (!policies)
            return 0;
        foreach (TXGInsurancePolicy p : policies)
        {
            if (!p)
                continue;
            if (p.status == TXGConstants.POLICY_ACTIVE && p.expiresAt > 0 && nowUnix >= p.expiresAt)
            {
                p.status = TXGConstants.POLICY_EXPIRED;
                p.voidReason = "EXPIRED";
                expiredNow = expiredNow + 1;
            }
        }
        if (expiredNow > 0)
            Save();
        return expiredNow;
    }

    int CountActive(int nowUnix)
    {
        RefreshExpirations(nowUnix);
        int cnt = 0;
        if (!policies)
            return 0;
        foreach (TXGInsurancePolicy p : policies)
        {
            if (p && p.status == TXGConstants.POLICY_ACTIVE)
                cnt = cnt + 1;
        }
        return cnt;
    }

    void Save()
    {
        if (!FileExist(TXGConstants.ROOT_DIR))
            MakeDirectory(TXGConstants.ROOT_DIR);
        if (!FileExist(TXGConstants.CONFIG_DIR))
            MakeDirectory(TXGConstants.CONFIG_DIR);
        if (!FileExist(TXGConstants.INSURANCE_DIR))
            MakeDirectory(TXGConstants.INSURANCE_DIR);

        JsonFileLoader<TXGPlayerPolicies>.JsonSaveFile(GetFilePath(ownerUid), this);
        TXGPlayerIndex.Get().Touch(ownerUid, "");
    }

    static ref TXGPlayerPolicies Load(string uid)
    {
        if (!FileExist(TXGConstants.INSURANCE_DIR))
            MakeDirectory(TXGConstants.INSURANCE_DIR);

        string path = GetFilePath(uid);
        TXGPlayerPolicies data = new TXGPlayerPolicies();
        data.ownerUid = uid;

        if (FileExist(path))
        {
            JsonFileLoader<TXGPlayerPolicies>.JsonLoadFile(path, data);
            if (data.ownerUid == "")
                data.ownerUid = uid;
            if (!data.policies)
                data.policies = new array<ref TXGInsurancePolicy>();
        }
        return data;
    }
}
