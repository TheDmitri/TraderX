class ETraderXInsuranceStatus
{
    static const string ACTIVE  = "ACTIVE";
    static const string CLAIMED = "CLAIMED";
    static const string VOIDED  = "VOIDED";
    static const string EXPIRED = "EXPIRED";
}

class TraderXInsurancePolicy
{
    string policyId;
    string playerUID;
    string vehicleType;
    int vehicleNetworkLow;
    int vehicleNetworkHigh;
    int premiumPaid;
    int payoutAmount;
    int vehiclePrice;
    int createdTimestamp;
    string status;
    string presetId;
    bool isInGarage;

    void TraderXInsurancePolicy()
    {
        status = ETraderXInsuranceStatus.ACTIVE;
        isInGarage = false;
    }

    static TraderXInsurancePolicy Create(string playerUID, string vehicleType, int netLow, int netHigh, int premium, int payout, int price, string presetId)
    {
        TraderXInsurancePolicy policy = new TraderXInsurancePolicy();
        policy.policyId = UUID.Generate();
        policy.playerUID = playerUID;
        policy.vehicleType = vehicleType;
        policy.vehicleNetworkLow = netLow;
        policy.vehicleNetworkHigh = netHigh;
        policy.premiumPaid = premium;
        policy.payoutAmount = payout;
        policy.vehiclePrice = price;
        policy.createdTimestamp = GetGame().GetTime() / 1000;
        policy.status = ETraderXInsuranceStatus.ACTIVE;
        policy.presetId = presetId;
        policy.isInGarage = false;
        return policy;
    }

    bool IsActive()
    {
        return status == ETraderXInsuranceStatus.ACTIVE;
    }

    bool IsClaimable()
    {
        return status == ETraderXInsuranceStatus.ACTIVE && !isInGarage;
    }

    bool IsExpired(int expiryDays)
    {
        if (expiryDays <= 0)
            return false;

        int now = GetGame().GetTime() / 1000;
        int expirySeconds = expiryDays * 86400;
        return (now - createdTimestamp) > expirySeconds;
    }

    void Void()
    {
        status = ETraderXInsuranceStatus.VOIDED;
    }

    void Claim()
    {
        status = ETraderXInsuranceStatus.CLAIMED;
    }

    void Expire()
    {
        status = ETraderXInsuranceStatus.EXPIRED;
    }
}
