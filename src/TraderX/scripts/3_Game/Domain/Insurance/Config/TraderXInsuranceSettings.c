class TraderXInsuranceSettings
{
    bool enabled = true;
    float premiumCoefficient = 0.10;     // 10% of vehicle buy price as insurance cost
    float payoutCoefficient = 0.80;      // 80% of vehicle buy price as payout on claim
    int maxActivePolicies = 3;           // max concurrent policies per player
    int policyExpiryDays = 30;           // 0 = no expiry
    bool autoScanOnRestart = true;       // scan for missing insured vehicles on server start
    bool allowClaimWhileInGarage = false; // false = vehicle in garage blocks claim

    static TraderXInsuranceSettings Load()
    {
        TraderXInsuranceSettings settings = new TraderXInsuranceSettings();

        if (FileExist(TRADERX_INSURANCE_CONFIG_FILE))
        {
            JsonFileLoader<TraderXInsuranceSettings>.JsonLoadFile(TRADERX_INSURANCE_CONFIG_FILE, settings);
            return settings;
        }

        // Generate default config
        settings.Save();
        return settings;
    }

    void Save()
    {
        JsonFileLoader<TraderXInsuranceSettings>.JsonSaveFile(TRADERX_INSURANCE_CONFIG_FILE, this);
    }
}
