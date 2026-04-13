class TraderXInsuranceService : ITraderXInsuranceProvider
{
    private static ref TraderXInsuranceService m_Instance;
    private ref TraderXInsuranceSettings m_Settings;

    static TraderXInsuranceService GetInstance()
    {
        if (!m_Instance)
            m_Instance = new TraderXInsuranceService();
        return m_Instance;
    }

    void TraderXInsuranceService()
    {
        m_Instance = this;
    }

    void Initialize()
    {
        m_Settings = TraderXInsuranceSettings.Load();

        if (m_Settings.enabled)
        {
            TraderXProviderRegistry.GetInstance().RegisterInsuranceProvider(this);
            GetTraderXLogger().LogInfo("[Insurance] Service initialized (premium: " + (m_Settings.premiumCoefficient * 100).ToString() + "%, payout: " + (m_Settings.payoutCoefficient * 100).ToString() + "%)");
        }
    }

    void RegisterRPCs()
    {
        if (GetGame().IsServer())
        {
            GetDayZularRPCManager().AddRPC("TraderX", "InsurePurchaseRequest", this, DayZularRPCType.Server);
            GetDayZularRPCManager().AddRPC("TraderX", "ClaimInsuranceRequest", this, DayZularRPCType.Server);
            GetDayZularRPCManager().AddRPC("TraderX", "GetPlayerPoliciesRequest", this, DayZularRPCType.Server);
        }
        else
        {
            GetDayZularRPCManager().AddRPC("TraderX", "InsurePurchaseResponse", this, DayZularRPCType.Client);
            GetDayZularRPCManager().AddRPC("TraderX", "ClaimInsuranceResponse", this, DayZularRPCType.Client);
            GetDayZularRPCManager().AddRPC("TraderX", "GetPlayerPoliciesResponse", this, DayZularRPCType.Client);
        }
    }

    TraderXInsuranceSettings GetSettings()
    {
        return m_Settings;
    }

    // ========================================================================
    // ITraderXInsuranceProvider implementation
    // ========================================================================

    override bool CanInsure(PlayerBase player, string vehicleType, int vehiclePrice)
    {
        if (!m_Settings || !m_Settings.enabled)
            return false;

        string uid = player.GetIdentity().GetPlainId();
        TraderXInsurancePolicyCollection collection = TraderXInsuranceRepository.Load(uid);
        return collection.GetActivePolicyCount() < m_Settings.maxActivePolicies;
    }

    override int CalculatePremium(int vehiclePrice)
    {
        return Math.Round(vehiclePrice * m_Settings.premiumCoefficient);
    }

    override int CalculatePayout(int vehiclePrice)
    {
        return Math.Round(vehiclePrice * m_Settings.payoutCoefficient);
    }

    override bool CreatePolicy(PlayerBase player, string vehicleType, int netLow, int netHigh, int vehiclePrice, string presetId)
    {
        string uid = player.GetIdentity().GetPlainId();
        TraderXInsurancePolicyCollection collection = TraderXInsuranceRepository.Load(uid);

        if (collection.GetActivePolicyCount() >= m_Settings.maxActivePolicies)
        {
            GetTraderXLogger().LogWarning("[Insurance] Max policies reached for: " + uid);
            return false;
        }

        int premium = CalculatePremium(vehiclePrice);
        int payout = CalculatePayout(vehiclePrice);

        TraderXInsurancePolicy policy = TraderXInsurancePolicy.Create(uid, vehicleType, netLow, netHigh, premium, payout, vehiclePrice, presetId);
        collection.AddPolicy(policy);
        TraderXInsuranceRepository.Save(uid, collection);

        GetTraderXLogger().LogInfo(string.Format("[Insurance] Policy created: %1 for %2 (premium: %3, payout: %4)", policy.policyId, vehicleType, premium, payout));
        return true;
    }

    override bool VoidPolicy(string policyId, string playerUID)
    {
        TraderXInsurancePolicyCollection collection = TraderXInsuranceRepository.Load(playerUID);
        TraderXInsurancePolicy policy = collection.GetPolicyById(policyId);

        if (!policy || !policy.IsActive())
            return false;

        policy.Void();
        TraderXInsuranceRepository.Save(playerUID, collection);

        GetTraderXLogger().LogInfo("[Insurance] Policy voided: " + policyId);
        return true;
    }

    override bool ClaimPolicy(PlayerBase player, string policyId, out int payoutAmount)
    {
        payoutAmount = 0;
        string uid = player.GetIdentity().GetPlainId();
        TraderXInsurancePolicyCollection collection = TraderXInsuranceRepository.Load(uid);
        TraderXInsurancePolicy policy = collection.GetPolicyById(policyId);

        if (!policy || !policy.IsActive())
            return false;

        // Check expiry
        if (policy.IsExpired(m_Settings.policyExpiryDays))
        {
            policy.Expire();
            TraderXInsuranceRepository.Save(uid, collection);
            return false;
        }

        // Block claim if vehicle is in garage
        if (policy.isInGarage && !m_Settings.allowClaimWhileInGarage)
        {
            GetTraderXLogger().LogWarning("[Insurance] Claim blocked — vehicle is in garage: " + policyId);
            return false;
        }

        // Verify the vehicle actually doesn't exist in the world
        Object vehicle = GetGame().GetObjectByNetworkId(policy.vehicleNetworkLow, policy.vehicleNetworkHigh);
        if (vehicle)
        {
            GetTraderXLogger().LogWarning("[Insurance] Claim denied — vehicle still exists: " + policyId);
            return false;
        }

        policy.Claim();
        TraderXInsuranceRepository.Save(uid, collection);

        payoutAmount = policy.payoutAmount;
        GetTraderXLogger().LogInfo(string.Format("[Insurance] Policy claimed: %1 — payout: %2", policyId, payoutAmount));
        return true;
    }

    override int GetActivePolicyCount(string playerUID)
    {
        TraderXInsurancePolicyCollection collection = TraderXInsuranceRepository.Load(playerUID);
        return collection.GetActivePolicyCount();
    }

    override bool IsVehicleInsured(int netLow, int netHigh)
    {
        // This is an expensive scan — use sparingly
        array<string> allUIDs = TraderXInsuranceRepository.GetAllPlayerUIDs();
        foreach (string uid : allUIDs)
        {
            TraderXInsurancePolicyCollection collection = TraderXInsuranceRepository.Load(uid);
            TraderXInsurancePolicy policy = collection.FindByVehicleNetwork(netLow, netHigh);
            if (policy)
                return true;
        }
        return false;
    }

    override void OnVehicleParkedInGarage(string playerUID, int netLow, int netHigh)
    {
        TraderXInsurancePolicyCollection collection = TraderXInsuranceRepository.Load(playerUID);
        TraderXInsurancePolicy policy = collection.FindByVehicleNetwork(netLow, netHigh);
        if (policy && policy.IsActive())
        {
            policy.isInGarage = true;
            TraderXInsuranceRepository.Save(playerUID, collection);
            GetTraderXLogger().LogDebug("[Insurance] Policy paused (garage): " + policy.policyId);
        }
    }

    override void OnVehicleParkedOutOfGarage(string playerUID, int netLow, int netHigh)
    {
        TraderXInsurancePolicyCollection collection = TraderXInsuranceRepository.Load(playerUID);
        TraderXInsurancePolicy policy = collection.FindByVehicleNetwork(netLow, netHigh);
        if (policy && policy.IsActive())
        {
            policy.isInGarage = false;
            TraderXInsuranceRepository.Save(playerUID, collection);
            GetTraderXLogger().LogDebug("[Insurance] Policy resumed (un-garaged): " + policy.policyId);
        }
    }

    override void ScanForMissingVehicles()
    {
        if (!m_Settings.autoScanOnRestart)
            return;

        GetTraderXLogger().LogInfo("[Insurance] Scanning for missing insured vehicles...");

        int missingCount = 0;
        array<string> allUIDs = TraderXInsuranceRepository.GetAllPlayerUIDs();

        foreach (string uid : allUIDs)
        {
            TraderXInsurancePolicyCollection collection = TraderXInsuranceRepository.Load(uid);
            bool changed = false;

            // Purge expired
            collection.PurgeExpired(m_Settings.policyExpiryDays);

            foreach (TraderXInsurancePolicy policy : collection.policies)
            {
                if (!policy.IsActive() || policy.isInGarage)
                    continue;

                Object vehicle = GetGame().GetObjectByNetworkId(policy.vehicleNetworkLow, policy.vehicleNetworkHigh);
                if (!vehicle)
                {
                    missingCount++;
                    GetTraderXLogger().LogInfo(string.Format("[Insurance] Missing vehicle detected — policy: %1, type: %2, owner: %3", policy.policyId, policy.vehicleType, uid));
                    changed = true;
                }
            }

            if (changed)
                TraderXInsuranceRepository.Save(uid, collection);
        }

        GetTraderXLogger().LogInfo(string.Format("[Insurance] Scan complete — %1 missing vehicles found", missingCount));
    }

    // ========================================================================
    // RPC Handlers
    // ========================================================================

    void InsurePurchaseRequest(PlayerIdentity sender, ParamsReadContext ctx)
    {
        Param3<int, int, string> data;
        if (!ctx.Read(data))
            return;

        PlayerBase player = TraderXHelper.GetPlayerByIdentity(sender);
        if (!player)
            return;

        int netLow = data.param1;
        int netHigh = data.param2;
        string productId = data.param3;

        TraderXProduct product = TraderXProductRepository.GetItemById(productId);
        if (!product)
        {
            GetDayZularRPCManager().SendRPC("TraderX", "InsurePurchaseResponse", new Param2<bool, string>(false, "Product not found"), true, sender);
            return;
        }

        int premium = CalculatePremium(product.buyPrice);

        // Deduct premium
        TStringArray currencies = new TStringArray();
        TraderXNpc npc = GetTraderXModule().GetSettings().GetNpcById(TraderXTradingService.GetInstance().GetNpcId());
        if (npc)
            currencies = npc.GetCurrenciesAccepted();

        string paymentPriority = GetTraderXModule().GetSettings().paymentPriority;

        if (!TraderXPaymentService.GetInstance().RemovePlayerMoney(player, premium, currencies, paymentPriority))
        {
            GetDayZularRPCManager().SendRPC("TraderX", "InsurePurchaseResponse", new Param2<bool, string>(false, "Not enough money for premium"), true, sender);
            return;
        }

        bool success = CreatePolicy(player, product.className, netLow, netHigh, product.buyPrice, "");
        string msg = TernaryString(success, "Vehicle insured! Premium: " + premium.ToString(), "Failed to create insurance policy");

        GetDayZularRPCManager().SendRPC("TraderX", "InsurePurchaseResponse", new Param2<bool, string>(success, msg), true, sender);
    }

    void InsurePurchaseResponse(PlayerIdentity sender, ParamsReadContext ctx)
    {
        // Client-side: handled by UI
    }

    void ClaimInsuranceRequest(PlayerIdentity sender, ParamsReadContext ctx)
    {
        Param1<string> data;
        if (!ctx.Read(data))
            return;

        PlayerBase player = TraderXHelper.GetPlayerByIdentity(sender);
        if (!player)
            return;

        int payoutAmount;
        bool success = ClaimPolicy(player, data.param1, payoutAmount);

        if (success && payoutAmount > 0)
        {
            TStringArray currencies = new TStringArray();
            string paymentPriority = GetTraderXModule().GetSettings().paymentPriority;
            TraderXPaymentService.GetInstance().AddPlayerMoney(player, payoutAmount, currencies, paymentPriority);
        }

        string msg = TernaryString(success, "Insurance claimed! Payout: " + payoutAmount.ToString(), "Claim failed — vehicle may still exist or policy is invalid");
        GetDayZularRPCManager().SendRPC("TraderX", "ClaimInsuranceResponse", new Param2<bool, string>(success, msg), true, sender);
    }

    void ClaimInsuranceResponse(PlayerIdentity sender, ParamsReadContext ctx)
    {
        // Client-side: handled by UI
    }

    void GetPlayerPoliciesRequest(PlayerIdentity sender, ParamsReadContext ctx)
    {
        string uid = sender.GetPlainId();
        TraderXInsurancePolicyCollection collection = TraderXInsuranceRepository.Load(uid);

        // Purge expired before sending
        collection.PurgeExpired(m_Settings.policyExpiryDays);
        TraderXInsuranceRepository.Save(uid, collection);

        GetDayZularRPCManager().SendRPC("TraderX", "GetPlayerPoliciesResponse", new Param1<ref array<ref TraderXInsurancePolicy>>(collection.policies), true, sender);
    }

    void GetPlayerPoliciesResponse(PlayerIdentity sender, ParamsReadContext ctx)
    {
        // Client-side: handled by UI
    }
}
