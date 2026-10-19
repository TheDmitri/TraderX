/**
 * TraderXGarage implementation of ITraderXGarageProvider.
 * Thin façade delegating to TXGGarageService — mirror of TXBPaymentProvider.
 *
 * Registered automatically when TraderXGarage loads and TraderX is present.
 * All calls are server-side, in-process.
 *
 * NOTE: only compiled when TraderX is present (requires the core interface + DTO).
 */
#ifdef TraderX
class TXGGarageProvider : ITraderXGarageProvider
{
    override bool AddVehicle(string playerUid, EntityAI vehicle)
    {
        return GetTXGGarageService().StoreVehicleEntity(playerUid, vehicle);
    }

    override bool AddVehicleByType(string playerUid, string className)
    {
        return GetTXGGarageService().StoreVehicleByType(playerUid, className);
    }

    override bool RemoveVehicle(string playerUid, string vehicleId)
    {
        return GetTXGGarageService().RemoveVehicleById(playerUid, vehicleId);
    }

    override EntityAI RetrieveVehicle(string playerUid, string vehicleId, vector pos, vector orientation)
    {
        return GetTXGGarageService().RetrieveVehicleById(playerUid, vehicleId, pos, orientation);
    }

    override int GetStoredCount(string playerUid)
    {
        return GetTXGGarageService().GetStoredCount(playerUid);
    }

    override int GetMaxSlots()
    {
        return GetTXGGarageService().GetMaxSlots();
    }

    override bool HasFreeSlot(string playerUid)
    {
        return GetTXGGarageService().HasFreeSlot(playerUid);
    }

    override array<ref TraderXGarageVehicleInfo> ListVehicles(string playerUid)
    {
        array<ref TraderXGarageVehicleInfo> result = new array<ref TraderXGarageVehicleInfo>();
        if (playerUid == "")
            return result;

        TXGGarageData data = TXGGarageData.Load(playerUid, "");
        foreach (TXGVehicleData v : data.vehicles)
        {
            if (!v)
                continue;
            TraderXGarageVehicleInfo info = new TraderXGarageVehicleInfo();
            info.vehicleId = v.vehicleId;
            info.className = v.className;
            info.displayName = TXGGarageService.GetDisplayName(v.className);
            info.healthLevel = TXGGarageService.ConvertHealthToHealthLevel(v.className, v.health);
            result.Insert(info);
        }
        return result;
    }

    override bool IsResaleBlocked(EntityAI vehicle)
    {
        TXGInsuranceService ins = GetTXGInsuranceService();
        if (!ins)
            return false;
        return ins.IsResaleBlocked(vehicle);
    }

    override void OnVehicleSold(EntityAI vehicle, string sellerUid)
    {
        TXGInsuranceService ins = GetTXGInsuranceService();
        if (ins)
            ins.OnVehicleSold(vehicle, sellerUid);
    }
}
#endif // TraderX
