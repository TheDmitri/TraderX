// TraderXGarage - Vehicle Insurance Service (Server-authoritative)
// Implements docs/handoff/INSURANCE_SPEC.md:
// - No money payout ever; claims place a fresh replacement vehicle in the owner's garage under an INSURANCE hold.
// - Only server-observed EEKilled triggers a claim; external deletion or trader sale voids the policy.
// - Economic guard: the deductible is never below the vehicle's trader sell price.

class TXGInsuranceService extends DayZularService
{
    void TXGInsuranceService()
    {
        if (GetGame().IsServer() || !GetGame().IsMultiplayer())
            GetDayZularEvents().OnMissionStart.Insert(OnMissionStart);
    }

    void ~TXGInsuranceService()
    {
        if (GetDayZularEvents() && GetDayZularEvents().OnMissionStart)
            GetDayZularEvents().OnMissionStart.Remove(OnMissionStart);
    }

    protected void OnMissionStart()
    {
        if (!GetGame().IsServer())
            return;
        ValidateConfigAtBoot();
    }

    static int GetNowUnix()
    {
        return CF_Date.Now().GetTimestamp();
    }

    // ============================================================
    //  CATALOG VALUE & ECONOMIC GUARD
    // ============================================================

    int GetVehicleCatalogValue(string className)
    {
        if (className == "")
            return -1;

#ifdef TraderX
        ITraderXCatalogProvider cat = TraderXProviderRegistry.GetInstance().GetCatalogProvider();
        if (cat)
        {
            int val = cat.GetVehicleValue(className);
            if (val > 0)
                return val;
        }
#endif

        TXGSettings settings = GetTXGConfigService().GetServerSettings();
        if (settings && settings.vehicleValues)
        {
            string target = className;
            target.ToLower();
            foreach (TXGVehiclePriceEntry entry : settings.vehicleValues)
            {
                if (!entry || entry.className == "")
                    continue;
                string c = entry.className;
                c.ToLower();
                if (c == target && entry.buyPrice > 0)
                    return entry.buyPrice;
            }
        }
        return -1;
    }

    int GetVehicleCatalogSellPrice(string className)
    {
        if (className == "")
            return -1;

#ifdef TraderX
        ITraderXCatalogProvider cat = TraderXProviderRegistry.GetInstance().GetCatalogProvider();
        if (cat)
        {
            int val = cat.GetVehicleSellPrice(className);
            if (val >= 0)
                return val;
        }
#endif

        TXGSettings settings = GetTXGConfigService().GetServerSettings();
        if (settings && settings.vehicleValues)
        {
            string target = className;
            target.ToLower();
            foreach (TXGVehiclePriceEntry entry : settings.vehicleValues)
            {
                if (!entry || entry.className == "")
                    continue;
                string c = entry.className;
                c.ToLower();
                if (c == target && entry.sellPrice >= 0)
                    return entry.sellPrice;
            }
        }
        return -1;
    }

    bool IsClassInsurable(string className, TXGInsuranceSettings insCfg)
    {
        if (!insCfg || !insCfg.enabled || className == "")
            return false;
        if (GetVehicleCatalogValue(className) <= 0)
            return false;
        if (!insCfg.insurableClasses || insCfg.insurableClasses.Count() == 0)
            return true;

        string target = className;
        target.ToLower();
        foreach (string allowed : insCfg.insurableClasses)
        {
            string a = allowed;
            a.ToLower();
            if (a == target)
                return true;
        }
        return false;
    }

    int ComputePremium(int coveredValue, int days, TXGInsuranceSettings insCfg)
    {
        if (!insCfg || coveredValue <= 0 || days <= 0)
            return 0;
        float raw = coveredValue * (insCfg.premiumPercentPerDay * 0.01) * days;
        int rounded = Math.Ceil(raw);
        if (rounded < 1)
            rounded = 1;
        return rounded;
    }

    int ComputeDeductible(int coveredValue, TXGInsuranceSettings insCfg)
    {
        if (!insCfg || coveredValue <= 0)
            return 0;
        int pctVal = Math.Ceil(coveredValue * (insCfg.deductiblePercent * 0.01));
        if (pctVal < insCfg.deductibleMin)
            return insCfg.deductibleMin;
        return pctVal;
    }

    // Deductible never below what a trader pays for the vehicle: destroy-and-claim can't beat selling
    // (spec 4.3 economic guard, enforced instead of only warned).
    int ComputeDeductibleFor(string className, int coveredValue, TXGInsuranceSettings insCfg)
    {
        int ded = ComputeDeductible(coveredValue, insCfg);
        int sellPrice = GetVehicleCatalogSellPrice(className);
        if (sellPrice > ded)
            return sellPrice;
        return ded;
    }

    protected void ValidateConfigAtBoot()
    {
        TXGSettings settings = GetTXGConfigService().GetServerSettings();
        if (!settings || !settings.insurance || !settings.insurance.enabled)
            return;

        TXGInsuranceSettings ins = settings.insurance;
        if (settings.vehicleValues)
        {
            foreach (TXGVehiclePriceEntry entry : settings.vehicleValues)
            {
                if (!entry || entry.buyPrice <= 0)
                    continue;
                int ded = ComputeDeductible(entry.buyPrice, ins);
                if (entry.sellPrice > 0 && ded < entry.sellPrice)
                {
                    GetTXGLog().LogInfo("[TXG] Insurance deductible for " + entry.className + " raised from " + ded.ToString() + " to its sell price " + entry.sellPrice.ToString());
                }
            }
        }
    }

    // ============================================================
    //  SUBSCRIBE / EXTEND (Garage UI -> stored vehicle)
    // ============================================================

    string SubscribeStoredVehicle(PlayerBase player, string vehicleId, int days, out bool success)
    {
        success = false;
        if (!GetGame().IsServer() || !player || !player.GetIdentity())
            return "#txg_err_server";

        TXGSettings settings = GetTXGConfigService().GetServerSettings();
        if (!settings || !settings.insurance || !settings.insurance.enabled)
            return "#txg_err_ins_disabled";

        TXGInsuranceSettings insCfg = settings.insurance;
        int clampedDays = Math.Clamp(days, insCfg.minDays, insCfg.maxDays);

        string uid = player.GetIdentity().GetPlainId();
        string name = player.GetIdentity().GetName();

        if (!GetTXGGarageService().IsAtGarage(player))
            return "#txg_err_not_at_garage";

        TXGGarageData garage = TXGGarageData.Load(uid, name);
        TXGVehicleData record = garage.FindVehicle(vehicleId);
        if (!record)
            return "#txg_err_not_in_garage";

        if (record.holdReason != "")
            return "#txg_err_release_hold";

        if (!IsClassInsurable(record.className, insCfg))
            return "#txg_err_not_eligible";

        int coveredValue = GetVehicleCatalogValue(record.className);
        if (coveredValue <= 0)
            return "#txg_err_no_catalog_value";


        int nowUnix = GetNowUnix();
        TXGPlayerPolicies playerPolicies = TXGPlayerPolicies.Load(uid);
        playerPolicies.RefreshExpirations(nowUnix);

        TXGInsurancePolicy existing = playerPolicies.FindPolicy(record.policyId);
        bool isExtension = (existing && existing.status == TXGConstants.POLICY_ACTIVE);

        if (!isExtension && playerPolicies.CountActive(nowUnix) >= insCfg.maxActivePoliciesPerPlayer)
            return TXGText.Msg("#txg_err_max_policies", insCfg.maxActivePoliciesPerPlayer.ToString());

        int premium = ComputePremium(coveredValue, clampedDays, insCfg);
        if (!GetTXGGarageService().ChargeFee(player, premium, TXGConstants.FEE_LABEL_INSURANCE))
            return TXGText.Msg("#txg_err_no_money_premium", premium.ToString());

        int addedSeconds = clampedDays * 86400;
        if (isExtension)
        {
            existing.premiumPaid = existing.premiumPaid + premium;
            existing.expiresAt = existing.expiresAt + addedSeconds;
            playerPolicies.Save();
            GetTXGLog().LogInfo("[TXG INSURANCE] EXTEND owner=" + uid + " policy=" + existing.policyId + " class=" + record.className + " days=+" + clampedDays.ToString() + " premium=" + premium.ToString());
            success = true;
            return TXGText.Msg("#txg_msg_ins_extended", clampedDays.ToString());
        }

        TXGInsurancePolicy pol = new TXGInsurancePolicy();
        pol.policyId = playerPolicies.AllocatePolicyId();
        pol.ownerUid = uid;
        pol.vehicleClassName = record.className;
        pol.vehicleClassName.ToLower();
        pol.coveredValue = coveredValue;
        pol.premiumPaid = premium;
        pol.startedAt = nowUnix;
        pol.expiresAt = nowUnix + addedSeconds;
        pol.status = TXGConstants.POLICY_ACTIVE;

        playerPolicies.policies.Insert(pol);
        playerPolicies.Save();

        record.policyId = pol.policyId;
        record.lockOwnerUid = uid;
        garage.Save();

        GetTXGLog().LogInfo("[TXG INSURANCE] SUBSCRIBE owner=" + uid + " policy=" + pol.policyId + " class=" + record.className + " value=" + coveredValue.ToString() + " premium=" + premium.ToString() + " deductible=" + ComputeDeductibleFor(record.className, coveredValue, insCfg).ToString());
        success = true;
        return TXGText.Msg("#txg_msg_insured", clampedDays.ToString());
    }

    // ============================================================
    //  SERVER HOOKS: DESTROYED / DELETED / SOLD / RESALE CHECK
    // ============================================================

    void OnVehicleDestroyed(EntityAI vehicle)
    {
        if (!GetGame().IsServer() || !vehicle)
            return;

        TXGSettings settings = GetTXGConfigService().GetServerSettings();
        if (!settings || !settings.insurance || !settings.insurance.enabled)
            return;

        TXGInsuranceSettings insCfg = settings.insurance;
        if (insCfg.recoveryMode == "NONE")
            return;

        string policyId;
        string ownerUid;
        int stampState;
        int noResaleUntil;
        if (!TXGVehicleStampHelper.ReadStamp(vehicle, policyId, ownerUid, stampState, noResaleUntil))
            return;

        if (stampState == TXGConstants.STAMP_CLAIMED || ownerUid == "")
            return;

        int nowUnix = GetNowUnix();
        string vehClass = vehicle.GetType();
        int coveredValue = GetVehicleCatalogValue(vehClass);

        TXGPlayerPolicies playerPolicies = TXGPlayerPolicies.Load(ownerUid);
        playerPolicies.RefreshExpirations(nowUnix);

        int cooldownSec = insCfg.claimCooldownHours * 3600;
        if (cooldownSec > 0 && playerPolicies.lastClaimAt > 0 && (nowUnix - playerPolicies.lastClaimAt) < cooldownSec)
        {
            GetTXGLog().LogInfo("[TXG INSURANCE] CLAIM_REFUSED_COOLDOWN owner=" + ownerUid + " policy=" + policyId + " class=" + vehClass);
            return;
        }

        if (insCfg.recoveryMode == "INSURED_ONLY")
        {
            if (stampState != TXGConstants.STAMP_INSURED || policyId == "")
                return;

            TXGInsurancePolicy pol = playerPolicies.FindPolicy(policyId);
            if (!pol || pol.status != TXGConstants.POLICY_ACTIVE || pol.ownerUid != ownerUid)
            {
                GetTXGLog().LogInfo("[TXG INSURANCE] CLAIM_REFUSED_INACTIVE owner=" + ownerUid + " policy=" + policyId + " class=" + vehClass);
                return;
            }

            coveredValue = pol.coveredValue;
            pol.status = TXGConstants.POLICY_CLAIMED;
            pol.claimedAt = nowUnix;
        }

        if (coveredValue <= 0)
            coveredValue = 2000;

        int deductible = ComputeDeductibleFor(vehClass, coveredValue, insCfg);
        playerPolicies.lastClaimAt = nowUnix;
        playerPolicies.Save();

        // Mark wreck stamp CLAIMED so the standing wreck can never claim again.
        TXGVehicleStampHelper.ApplyStamp(vehicle, policyId, ownerUid, TXGConstants.STAMP_CLAIMED, noResaleUntil);

        // Build a fresh replacement record in the owner's garage under an INSURANCE hold (no cargo!).
        TXGGarageData garage = TXGGarageData.Load(ownerUid, "");
        TXGVehicleData replacement = BuildFreshReplacementRecord(garage, vehClass, ownerUid, deductible, nowUnix, insCfg);
        garage.vehicles.Insert(replacement);
        garage.Save();

        GetTXGLog().LogInfo("[TXG INSURANCE] CLAIMED owner=" + ownerUid + " policy=" + policyId + " class=" + vehClass + " deductible=" + deductible.ToString() + " slot=" + replacement.vehicleId);
    }

    protected TXGVehicleData BuildFreshReplacementRecord(TXGGarageData garage, string className, string ownerUid, int deductible, int nowUnix, TXGInsuranceSettings insCfg)
    {
        TXGVehicleData rec = new TXGVehicleData();
        rec.vehicleId = garage.AllocateVehicleId();
        rec.className = className;
        rec.health = MiscGameplayFunctions.GetTypeMaxGlobalHealth(className);
        rec.fuelFraction = 1.0;
        rec.oilFraction = 1.0;
        rec.coolantFraction = 1.0;
        rec.brakeFraction = 1.0;
        rec.fluidsSaved = true;
        rec.timestamp = TXGGarageService.GetUtcStamp();
        rec.lockOwnerUid = ownerUid;
        rec.holdReason = TXGConstants.HOLD_INSURANCE;
        rec.releaseFee = deductible;
        rec.noResaleUntil = nowUnix + (insCfg.replacementNoResaleHours * 3600);
        rec.snapshot = new TXGItemSnapshot();
        rec.snapshot.item = className;

        if (insCfg.replacementSpec == "CATALOG_PRESET")
        {
#ifdef TraderX
            ITraderXCatalogProvider cat = TraderXProviderRegistry.GetInstance().GetCatalogProvider();
            if (cat)
            {
                array<string> presetParts = cat.GetDefaultVehicleAttachments(className);
                if (presetParts)
                {
                    foreach (string partCls : presetParts)
                    {
                        if (partCls == "")
                            continue;
                        TXGItemSnapshot partSnap = new TXGItemSnapshot();
                        partSnap.item = partCls;
                        partSnap.isAttachment = true;
                        partSnap.slot = InventorySlots.INVALID;
                        partSnap.health = MiscGameplayFunctions.GetTypeMaxGlobalHealth(partCls);
                        rec.snapshot.childrens.Insert(partSnap);
                    }
                }
            }
#endif
        }
        return rec;
    }

    void OnVehicleDeletedExternally(EntityAI vehicle)
    {
        if (!GetGame().IsServer() || !vehicle)
            return;

        string policyId;
        string ownerUid;
        int stampState;
        int noResaleUntil;
        if (!TXGVehicleStampHelper.ReadStamp(vehicle, policyId, ownerUid, stampState, noResaleUntil))
            return;

        if (policyId != "" && ownerUid != "")
            VoidPolicy(ownerUid, policyId, "DELETED");
    }

    void OnVehicleSold(EntityAI vehicle, string sellerUid)
    {
        if (!GetGame().IsServer() || !vehicle)
            return;

        string policyId;
        string ownerUid;
        int stampState;
        int noResaleUntil;
        TXGVehicleStampHelper.ReadStamp(vehicle, policyId, ownerUid, stampState, noResaleUntil);

        string targetOwner = ownerUid;
        if (targetOwner == "")
            targetOwner = sellerUid;

        if (policyId != "" && targetOwner != "")
            VoidPolicy(targetOwner, policyId, "SOLD");
    }

    bool IsResaleBlocked(EntityAI vehicle)
    {
        if (!vehicle)
            return false;

        string policyId;
        string ownerUid;
        int stampState;
        int noResaleUntil;
        if (!TXGVehicleStampHelper.ReadStamp(vehicle, policyId, ownerUid, stampState, noResaleUntil))
            return false;

        if (noResaleUntil <= 0)
            return false;

        return GetNowUnix() < noResaleUntil;
    }

    void VoidPolicy(string ownerUid, string policyId, string reason)
    {
        if (ownerUid == "" || policyId == "")
            return;

        TXGPlayerPolicies playerPolicies = TXGPlayerPolicies.Load(ownerUid);
        TXGInsurancePolicy pol = playerPolicies.FindPolicy(policyId);
        if (!pol || pol.status != TXGConstants.POLICY_ACTIVE)
            return;

        pol.status = TXGConstants.POLICY_VOID;
        pol.voidReason = reason;
        playerPolicies.Save();

        GetTXGLog().LogInfo("[TXG INSURANCE] VOID owner=" + ownerUid + " policy=" + policyId + " reason=" + reason);
    }
}

static ref TXGInsuranceService g_TXGInsuranceService;

TXGInsuranceService GetTXGInsuranceService()
{
    if (!g_TXGInsuranceService)
        g_TXGInsuranceService = TXGInsuranceService.Cast(DZlrServiceRegistry.GetInstance().GetService(TXGInsuranceService));
    return g_TXGInsuranceService;
}
