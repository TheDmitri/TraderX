// Dr Jones Trader (TM/Trader) -> TraderX catalog converter.
//
// Source: $profile:Trader\TraderConfig.txt  — a line-based text format:
//   <CurrencyName> #tm_ruble
//       <Currency> MoneyRuble1, 1
//   <Trader> Consume Trader              // trader id is sequential (0,1,2...)
//       <Category> Vegetables
//           ClassName, Quantity, Buyvalue, Sellvalue
// Quantity codes: * = max stack, M = magazine, W = weapon, S = steak, V/VNK = vehicle, K = key dup,
// or a literal number. Buyvalue -1 = not buyable, Sellvalue -1 = not sellable.
//
// We convert the CATALOG (categories + products + currencies). Trader NPC world positions are NOT
// in TraderConfig.txt (they live in the mod's object/marker files), so NPCs are not placed — assign
// the migrated categories to TraderX NPCs in TraderXGeneralSettings.json yourself.
class ConvertDrJonesTrader
{
    static const string SOURCE = "$profile:Trader/TraderConfig.txt";

    ref map<string, ref TraderXCategory> m_Categories;
    ref TraderXCurrencyTypeCollection m_Currencies;
    ref TraderXCurrencyType m_CurrentCurrencyType;

    void ConvertDrJonesTrader()
    {
        m_Categories = new map<string, ref TraderXCategory>();
        m_Currencies = new TraderXCurrencyTypeCollection();
    }

    static void ConvertIfPresent()
    {
        if (!FileExist(SOURCE))
            return;
        if (TraderXExternalMigration.IsDone("drjones"))
            return;

        ConvertDrJonesTrader converter = new ConvertDrJonesTrader();
        converter.Run();
    }

    void Run()
    {
        GetTraderXLogger().LogInfo("[TraderX Migration] Dr Jones Trader: converting " + SOURCE);
        TraderXExternalMigration.EnsureCatalogDirs();

        FileHandle fh = OpenFile(SOURCE, FileMode.READ);
        if (!fh)
        {
            GetTraderXLogger().LogError("[TraderX Migration] Dr Jones: cannot open " + SOURCE);
            return;
        }

        TraderXCategory currentCategory = null;
        int traderCount = 0;
        int productCount = 0;
        string raw;

        while (FGets(fh, raw) >= 0)
        {
            string line = TraderXExternalMigration.CleanLine(raw);
            if (line == "")
                continue;

            if (TraderXExternalMigration.StartsWith(line, "<CurrencyName>"))
            {
                m_CurrentCurrencyType = ParseCurrencyName(line);
                continue;
            }

            if (TraderXExternalMigration.StartsWith(line, "<Currency>"))
            {
                ParseCurrency(line);
                continue;
            }

            if (TraderXExternalMigration.StartsWith(line, "<Trader>"))
            {
                traderCount++;
                currentCategory = null;
                continue;
            }

            if (TraderXExternalMigration.StartsWith(line, "<Category>"))
            {
                currentCategory = GetOrCreateCategory(TraderXExternalMigration.AfterTag(line, "<Category>"));
                continue;
            }

            // Anything else with at least 4 comma fields is a product line.
            if (currentCategory && ParseProduct(line, currentCategory))
                productCount++;
        }

        CloseFile(fh);

        // Persist categories (products were saved as they were parsed).
        foreach (string cname, TraderXCategory cat : m_Categories)
        {
            TraderXCategoryRepository.Save(cat);
        }

        if (m_Currencies.currencyTypes.Count() > 0)
            TraderXCurrencyRepository.Save(m_Currencies);

        GetTraderXLogger().LogInfo("[TraderX Migration] Dr Jones: done. traders=" + traderCount + " categories=" + m_Categories.Count() + " products=" + productCount);
        GetTraderXLogger().LogInfo("[TraderX Migration] Dr Jones: NOTE trader NPC positions are not in TraderConfig.txt — assign categories to NPCs in TraderXGeneralSettings.json and place them in-world.");

        TraderXExternalMigration.MarkDone("drjones", "Dr Jones TraderConfig.txt migrated. categories=" + m_Categories.Count() + " products=" + productCount);
    }

    TraderXCurrencyType ParseCurrencyName(string line)
    {
        string val = TraderXExternalMigration.AfterTag(line, "<CurrencyName>");
        val.Replace("#", "");
        val.Replace("tm_", "");
        val.ToUpper();
        if (val == "")
            val = "CURRENCY";

        TraderXCurrencyType t = new TraderXCurrencyType(val);
        m_Currencies.currencyTypes.Insert(t);
        return t;
    }

    void ParseCurrency(string line)
    {
        if (!m_CurrentCurrencyType)
            return;

        string val = TraderXExternalMigration.AfterTag(line, "<Currency>");
        TStringArray parts = new TStringArray();
        val.Split(",", parts);
        if (parts.Count() < 2)
            return;

        string cls = parts[0];
        cls.Trim();
        string amt = parts[1];
        amt.Trim();
        if (cls == "")
            return;

        m_CurrentCurrencyType.AddCurrency(cls, amt.ToInt());
    }

    TraderXCategory GetOrCreateCategory(string name)
    {
        if (m_Categories.Contains(name))
            return m_Categories.Get(name);

        string catId = TraderXCategoryId.GenerateCategoryId(name);
        TraderXCategory cat = TraderXCategory.CreateCategory(name, 0, catId);
        m_Categories.Set(name, cat);
        return cat;
    }

    bool ParseProduct(string line, TraderXCategory cat)
    {
        TStringArray parts = new TStringArray();
        line.Split(",", parts);
        if (parts.Count() < 4)
            return false;

        string className = parts[0];
        className.Trim();
        string qty = parts[1];
        qty.Trim();
        string buyStr = parts[2];
        buyStr.Trim();
        string sellStr = parts[3];
        sellStr.Trim();

        if (className == "")
            return false;

        int buyPrice = buyStr.ToInt();
        int sellPrice = sellStr.ToInt();
        int tradeQuantity = MapQuantity(qty);

        string productId = TraderXProductId.GenerateProductId(className);
        if (productId == string.Empty)
            return false;

        TraderXProduct product = TraderXProduct.CreateProduct(className, 1.0, -1, tradeQuantity, buyPrice, sellPrice, 0.0, 0, null, null, productId);
        if (!product)
            return false;

        TraderXProductRepository.Save(product);
        cat.AddProduct(product);
        return true;
    }

    int MapQuantity(string qty)
    {
        // '*' (max stack) and 'M' (magazine) -> sell/buy FULL.
        if (qty == "*" || qty == "M")
            return TraderXExternalMigration.PackTradeQuantity(0x2, 0x2, 0, 0);

        int n = qty.ToInt();
        if (n > 0)
            return TraderXExternalMigration.PackTradeQuantity(0x4, 0x4, n, n);

        // V, VNK, W, S, K, or unparsable -> single unit.
        return TraderXExternalMigration.PackTradeQuantity(0x4, 0x4, 1, 1);
    }
}
