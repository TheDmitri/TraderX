class TraderXInsurancePolicyCollection
{
    string playerUID;
    ref array<ref TraderXInsurancePolicy> policies;

    void TraderXInsurancePolicyCollection(string uid = "")
    {
        playerUID = uid;
        policies = new array<ref TraderXInsurancePolicy>();
    }

    void AddPolicy(TraderXInsurancePolicy policy)
    {
        policies.Insert(policy);
    }

    TraderXInsurancePolicy GetPolicyById(string policyId)
    {
        foreach (TraderXInsurancePolicy policy : policies)
        {
            if (policy.policyId == policyId)
                return policy;
        }
        return null;
    }

    TraderXInsurancePolicy FindByVehicleNetwork(int netLow, int netHigh)
    {
        foreach (TraderXInsurancePolicy policy : policies)
        {
            if (policy.IsActive() && policy.vehicleNetworkLow == netLow && policy.vehicleNetworkHigh == netHigh)
                return policy;
        }
        return null;
    }

    int GetActivePolicyCount()
    {
        int count = 0;
        foreach (TraderXInsurancePolicy policy : policies)
        {
            if (policy.IsActive())
                count++;
        }
        return count;
    }

    void PurgeExpired(int expiryDays)
    {
        foreach (TraderXInsurancePolicy policy : policies)
        {
            if (policy.IsActive() && policy.IsExpired(expiryDays))
                policy.Expire();
        }
    }
}
