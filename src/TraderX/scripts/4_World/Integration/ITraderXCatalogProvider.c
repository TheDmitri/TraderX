/**
 * Port: catalog provider. Server-side, in-process calls.
 * Used by TraderXGarage (Insurance coverage value, deductible economic guard, replacement preset).
 */
class ITraderXCatalogProvider
{
    int GetVehicleValue(string className) { return -1; }
    int GetVehicleSellPrice(string className) { return -1; }
    array<string> GetDefaultVehicleAttachments(string className) { return null; }
}

class TraderXCoreCatalogProvider : ITraderXCatalogProvider
{
    protected TraderXProduct FindProductByClass(string className)
    {
        if (className == "")
            return null;
        string target = className;
        target.ToLower();

        array<ref TraderXProduct> products = TraderXProductRepository.GetProducts();
        if (!products)
            return null;

        foreach (TraderXProduct prod : products)
        {
            if (!prod || prod.className == "")
                continue;
            string pClass = prod.className;
            pClass.ToLower();
            if (pClass == target)
                return prod;
        }
        return null;
    }

    override int GetVehicleValue(string className)
    {
        TraderXProduct prod = FindProductByClass(className);
        if (!prod || prod.buyPrice < 0)
            return -1;
        return prod.buyPrice;
    }

    override int GetVehicleSellPrice(string className)
    {
        TraderXProduct prod = FindProductByClass(className);
        if (!prod || prod.sellPrice < 0)
            return -1;
        return prod.sellPrice;
    }

    // Parts of the product's default preset (one entry per fitted part, so 4 wheels = 4 entries).
    // Falls back to the product's attachment list when the vehicle has no preset.
    override array<string> GetDefaultVehicleAttachments(string className)
    {
        array<string> result = new array<string>();
        TraderXProduct prod = FindProductByClass(className);
        if (!prod)
            return result;

        array<string> partIds = prod.attachments;
        TraderXPresets presets = TraderXPresetsService.GetInstance().GetServerPresets(prod.productId);
        if (presets)
        {
            TraderXPreset preset = presets.GetDefaultPreset();
            if (preset && preset.attachments && preset.attachments.Count() > 0)
                partIds = preset.attachments;
        }
        if (!partIds)
            return result;

        foreach (string attId : partIds)
        {
            TraderXProduct attProd = TraderXProductRepository.GetItemById(attId);
            if (attProd && attProd.className != "")
                result.Insert(attProd.className);
        }
        return result;
    }
}
