// Conversions between the CSV rows and the product/category/preset models.
// The only place that maps CSV columns to runtime fields: compiler, legacy export and JSON source all use it.
class TraderXCsvMapper
{
    static TraderXCompiledProduct ToCompiledProduct(TraderXCsvProduct csv)
    {
        TraderXCompiledProduct product = new TraderXCompiledProduct();
        product.productId = csv.productKey;
        product.className = csv.className;
        product.buyPrice = csv.buyPrice;
        product.sellPrice = csv.sellPrice;
        product.maxStock = csv.maxStock;
        product.coefficient = csv.coefficient;
        product.tradeQuantity = TraderXBitfieldPacker.PackTradeQuantity(csv.buyQtyMode, csv.buyQtyValue, csv.sellQtyMode, csv.sellQtyValue);
        product.stockSettings = TraderXBitfieldPacker.PackStockSettings(csv.destockCoefficient, csv.stockBehaviorAtRestart);
        product.attachments = CopyList(csv.attachments);
        product.variants = CopyList(csv.variants);
        return product;
    }

    static TraderXCsvProduct FromProduct(string productId, TraderXJsonProduct product)
    {
        TraderXCsvProduct csv = new TraderXCsvProduct();
        csv.productKey = productId;
        csv.className = product.className;
        csv.buyPrice = product.buyPrice;
        csv.sellPrice = product.sellPrice;
        csv.maxStock = product.maxStock;
        csv.coefficient = product.coefficient;

        string buyMode;
        float buyValue;
        string sellMode;
        float sellValue;
        TraderXBitfieldPacker.UnpackTradeQuantity(product.tradeQuantity, buyMode, buyValue, sellMode, sellValue);
        csv.buyQtyMode = buyMode;
        csv.buyQtyValue = buyValue;
        csv.sellQtyMode = sellMode;
        csv.sellQtyValue = sellValue;

        float destock;
        int behavior;
        TraderXBitfieldPacker.UnpackStockSettings(product.stockSettings, destock, behavior);
        csv.destockCoefficient = destock;
        // 3 is unused by the runtime and behaves like 0 (keep)
        if (behavior == 3)
            behavior = 0;
        csv.stockBehaviorAtRestart = behavior;

        csv.attachments = CopyList(product.attachments);
        csv.variants = CopyList(product.variants);
        return csv;
    }

    static TraderXCompiledCategory ToCompiledCategory(TraderXCsvCategory csv)
    {
        TraderXCompiledCategory category = new TraderXCompiledCategory();
        category.categoryId = csv.categoryKey;
        category.categoryName = csv.categoryName;
        category.icon = csv.icon;
        category.isVisible = csv.isVisible;
        category.licensesRequired = CopyList(csv.licensesRequired);
        category.productIds = CopyList(csv.productKeys);
        return category;
    }

    static TraderXCsvCategory FromCategory(string categoryId, TraderXJsonCategory category)
    {
        TraderXCsvCategory csv = new TraderXCsvCategory();
        csv.categoryKey = categoryId;
        csv.categoryName = category.categoryName;
        csv.icon = category.icon;
        csv.isVisible = category.isVisible;
        csv.licensesRequired = CopyList(category.licensesRequired);
        csv.productKeys = CopyList(category.productIds);
        return csv;
    }

    static array<ref TraderXCsvPreset> FromPresets(TraderXPresets presets)
    {
        array<ref TraderXCsvPreset> rows = new array<ref TraderXCsvPreset>;
        if (!presets || !presets.presets)
            return rows;

        foreach (int i, TraderXPreset preset : presets.presets)
        {
            if (!preset)
                continue;
            TraderXCsvPreset csv = new TraderXCsvPreset();
            csv.presetKey = preset.presetId;
            if (csv.presetKey == "")
                csv.presetKey = string.Format("preset_%1_%2", presets.productId, i + 1);
            csv.productKey = presets.productId;
            csv.presetName = preset.presetName;
            csv.isDefault = preset.presetId != "" && preset.presetId == presets.defaultPresetId;
            csv.attachments = CopyList(preset.attachments);
            rows.Insert(csv);
        }
        return rows;
    }

    static array<string> CopyList(array<string> source)
    {
        array<string> copy = new array<string>;
        if (source)
            copy.Copy(source);
        return copy;
    }
}
