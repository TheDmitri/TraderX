// DayZ-Expansion-Market -> TraderX catalog converter.
//
// Source: $profile:ExpansionMod\Market\*.json  — one file per category. Schema (v12):
//   { m_Version, DisplayName, Icon, Color, IsExchange, InitStockPercent,
//     Items: [ { ClassName, MaxPriceThreshold, MinPriceThreshold, SellPricePercent,
//                MaxStockThreshold, MinStockThreshold, QuantityPercent, SpawnAttachments[], Variants[] } ] }
//
// Mapping notes / known v1 limitations (all logged with counts):
//  - Expansion prices are DYNAMIC (stock-curve between Min/MaxPriceThreshold). TraderX uses a single
//    buy price + coefficient, so we take MaxPriceThreshold as the buy price (exact when Min==Max).
//  - SellPricePercent is a fraction of buy price (-1 = not sellable) -> sellPrice = round(buy * pct).
//  - SpawnAttachments / Variants are class names; TraderX wants product-id references, so they are
//    dropped in this pass (count logged). Re-add via TraderX presets/attachments if needed.
//  - Trader<->category assignment and NPC positions live in Expansion's Traders\ + mission objects,
//    not in the category files; assign categories to TraderX NPCs in TraderXGeneralSettings.json.

class expMarketItem
{
    string ClassName;
    int MaxPriceThreshold;
    int MinPriceThreshold;
    float SellPricePercent;
    int MaxStockThreshold;
    int MinStockThreshold;
    int QuantityPercent;
    ref array<string> SpawnAttachments;
    ref array<string> Variants;

    void expMarketItem()
    {
        SpawnAttachments = new array<string>();
        Variants = new array<string>();
    }
}

class expMarketCategory
{
    int m_Version;
    string DisplayName;
    string Icon;
    string Color;
    int IsExchange;
    float InitStockPercent;
    ref array<ref expMarketItem> Items;

    void expMarketCategory()
    {
        Items = new array<ref expMarketItem>();
    }
}

class ConvertExpansionMarket
{
    static const string MARKET_DIR = "$profile:ExpansionMod/Market/";

    int m_CatCount;
    int m_ProdCount;
    int m_DroppedAttachments;
    int m_DroppedVariants;
    int m_FlattenedPrices;

    static void ConvertIfPresent()
    {
        if (TraderXExternalMigration.IsDone("expansion"))
            return;

        // Bail quietly if no Expansion market category files are present.
        string probe;
        FileAttr probeAttr;
        FindFileHandle probeHandle = FindFile(MARKET_DIR + "*.json", probe, probeAttr, FindFileFlags.ALL);
        if (!probeHandle)
            return;
        CloseFindFile(probeHandle);

        ConvertExpansionMarket converter = new ConvertExpansionMarket();
        converter.Run();
    }

    void Run()
    {
        GetTraderXLogger().LogInfo("[TraderX Migration] Expansion Market: scanning " + MARKET_DIR);
        TraderXExternalMigration.EnsureCatalogDirs();

        string filename;
        FileAttr attr;
        FindFileHandle handle = FindFile(MARKET_DIR + "*.json", filename, attr, FindFileFlags.ALL);
        if (!handle)
            return;

        ProcessFile(filename);

        while (FindNextFile(handle, filename, attr))
        {
            ProcessFile(filename);
        }
        CloseFindFile(handle);

        GetTraderXLogger().LogInfo("[TraderX Migration] Expansion Market: done. categories=" + m_CatCount + " products=" + m_ProdCount + " attachmentsDropped=" + m_DroppedAttachments + " variantsDropped=" + m_DroppedVariants + " dynamicPricesFlattened=" + m_FlattenedPrices);
        GetTraderXLogger().LogInfo("[TraderX Migration] Expansion Market: NOTE assign migrated categories to TraderX NPCs in TraderXGeneralSettings.json (trader positions are not in the market category files).");

        TraderXExternalMigration.MarkDone("expansion", "Expansion Market migrated. categories=" + m_CatCount + " products=" + m_ProdCount);
    }

    void ProcessFile(string filename)
    {
        expMarketCategory cat = new expMarketCategory();
        TraderXJsonLoader<expMarketCategory>.LoadFromFile(MARKET_DIR + filename, cat);

        // Not a category file (or empty) — skip so we don't create junk categories.
        if (!cat || !cat.Items || cat.Items.Count() == 0)
            return;

        string displayName = cat.DisplayName;
        if (displayName == string.Empty)
        {
            displayName = filename;
            displayName.Replace(".json", "");
        }

        string categoryId = TraderXCategoryId.GenerateCategoryId(displayName);
        if (categoryId == string.Empty)
            return;

        TraderXCategory txCat = TraderXCategory.CreateCategory(displayName, 0, categoryId);

        foreach (expMarketItem item : cat.Items)
        {
            if (MapItem(item, txCat))
                m_ProdCount++;
        }

        TraderXCategoryRepository.Save(txCat);
        m_CatCount++;
    }

    bool MapItem(expMarketItem item, TraderXCategory txCat)
    {
        if (!item || item.ClassName == string.Empty)
            return false;

        int buyPrice = item.MaxPriceThreshold;
        if (item.MinPriceThreshold != item.MaxPriceThreshold)
            m_FlattenedPrices++;

        int sellPrice = -1;
        if (item.SellPricePercent >= 0)
        {
            float pct = item.SellPricePercent;
            if (pct > 1.0)
                pct = pct / 100.0;
            sellPrice = (int)Math.Round(buyPrice * pct);
        }

        int maxStock = -1;
        if (item.MaxStockThreshold > 0)
            maxStock = item.MaxStockThreshold;

        // Sell/buy full stack by default (Expansion QuantityPercent nuance not represented in TraderX).
        int tradeQuantity = TraderXExternalMigration.PackTradeQuantity(0x2, 0x2, 0, 0);

        if (item.SpawnAttachments && item.SpawnAttachments.Count() > 0)
            m_DroppedAttachments += item.SpawnAttachments.Count();
        if (item.Variants && item.Variants.Count() > 0)
            m_DroppedVariants += item.Variants.Count();

        string productId = TraderXProductId.GenerateProductId(item.ClassName);
        if (productId == string.Empty)
            return false;

        TraderXProduct product = TraderXProduct.CreateProduct(item.ClassName, 1.0, maxStock, tradeQuantity, buyPrice, sellPrice, 0.0, 0, null, null, productId);
        if (!product)
            return false;

        TraderXProductRepository.Save(product);
        txCat.AddProduct(product);
        return true;
    }
}
