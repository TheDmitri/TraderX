// Shared helpers for migrating OTHER trader mods (Dr Jones Trader, Expansion Market) into TraderX.
//
// Unlike the TraderPlus V1 converter, we do NOT delete/move the source — those mods may still be
// loaded, and their config dirs are theirs. Instead we drop a one-shot sentinel in TraderXConfig\
// so a converter runs exactly once. Delete the sentinel to force a re-run.
class TraderXExternalMigration
{
    static bool IsDone(string key)
    {
        return FileExist(TRADERX_CONFIG_DIR_SERVER + ".migrated_" + key);
    }

    static void MarkDone(string key, string note)
    {
        if (!FileExist(TRADERX_CONFIG_DIR_SERVER))
            MakeDirectory(TRADERX_CONFIG_DIR_SERVER);

        FileHandle fh = OpenFile(TRADERX_CONFIG_DIR_SERVER + ".migrated_" + key, FileMode.WRITE);
        if (fh)
        {
            FPrintln(fh, note);
            CloseFile(fh);
        }
    }

    // Imports add to the catalog but never overwrite what the admin already has.
    static bool SaveProductIfNew(TraderXProduct product)
    {
        if (FileExist(string.Format(TRADERX_PRODUCT_FILE, product.productId)))
            return false;

        TraderXProductRepository.Save(product);
        return true;
    }

    static bool SaveCategoryIfNew(TraderXCategory category)
    {
        if (FileExist(string.Format(TRADERX_CATEGORY_FILE, category.categoryId)))
        {
            GetTraderXLogger().LogWarning("[TraderX Migration] Category " + category.categoryId + " already exists - kept the existing file, imported products are not added to it");
            return false;
        }

        TraderXCategoryRepository.Save(category);
        return true;
    }

    static void EnsureCatalogDirs()
    {
        // MakeDirectory does not create parents.
        if (!FileExist(TRADERX_CONFIG_ROOT_SERVER))
            MakeDirectory(TRADERX_CONFIG_ROOT_SERVER);
        if (!FileExist(TRADERX_CONFIG_DIR_SERVER))
            MakeDirectory(TRADERX_CONFIG_DIR_SERVER);
        if (!FileExist(TRADERX_DB_DIR_SERVER))
            MakeDirectory(TRADERX_DB_DIR_SERVER);
        if (!FileExist(TRADERX_STOCK_DIR))
            MakeDirectory(TRADERX_STOCK_DIR);
        if (!FileExist(TRADERX_CATEGORIES_DIR))
            MakeDirectory(TRADERX_CATEGORIES_DIR);
        if (!FileExist(TRADERX_PRODUCTS_DIR))
            MakeDirectory(TRADERX_PRODUCTS_DIR);
    }

    // Strip a trailing "//" line comment and surrounding whitespace. Returns "" for comment-only lines.
    // Ids must not collide with NPCs already in the settings nor with the reserved garage/bank ids.
    static int NextFreeNpcId(TraderXGeneralSettings settings)
    {
        int id = 1;
        foreach (TraderXNpc existing : settings.traders)
        {
            if (existing && existing.npcId >= id && existing.npcId != 125 && existing.npcId != 126)
                id = existing.npcId + 1;
        }
        if (id == 125 || id == 126)
            id = 127;
        return id;
    }

    static string CleanLine(string line)
    {
        // Trim() only strips spaces; Dr Jones files are tab-aligned with CRLF endings.
        line.Replace("\t", " ");
        line.Replace("\r", "");
        int idx = line.IndexOf("//");
        if (idx == 0)
            return "";
        if (idx > 0)
            line = line.Substring(0, idx);
        line.TrimInPlace();
        return line;
    }

    static bool StartsWith(string s, string prefix)
    {
        return s.IndexOf(prefix) == 0;
    }

    // Returns the text after a leading tag (e.g. "<Category>"), trimmed.
    static string AfterTag(string line, string tag)
    {
        string v = line.Substring(tag.Length(), line.Length() - tag.Length());
        v.TrimInPlace();
        return v;
    }

    // Same trade-quantity bit packing the TraderPlus V1 converter uses.
    // modes: 0x0 EMPTY/NO_MATTER, 0x1 (sell empty), 0x2 FULL, 0x3 COEFFICIENT, 0x4 STATIC.
    static int PackTradeQuantity(int buyMode, int sellMode, int buyQuantity, int sellQuantity)
    {
        return sellMode | (buyMode << 3) | ((sellQuantity & 0x1FFF) << 6) | (buyQuantity << 19);
    }
}
