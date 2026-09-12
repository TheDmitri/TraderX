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

    static void EnsureCatalogDirs()
    {
        if (!FileExist(TRADERX_CATEGORIES_DIR))
            MakeDirectory(TRADERX_CATEGORIES_DIR);
        if (!FileExist(TRADERX_PRODUCTS_DIR))
            MakeDirectory(TRADERX_PRODUCTS_DIR);
    }

    // Strip a trailing "//" line comment and surrounding whitespace. Returns "" for comment-only lines.
    static string CleanLine(string line)
    {
        int idx = line.IndexOf("//");
        if (idx == 0)
            return "";
        if (idx > 0)
            line = line.Substring(0, idx);
        line.Trim();
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
        v.Trim();
        return v;
    }

    // Same trade-quantity bit packing the TraderPlus V1 converter uses.
    // modes: 0x0 EMPTY/NO_MATTER, 0x1 (sell empty), 0x2 FULL, 0x3 COEFFICIENT, 0x4 STATIC.
    static int PackTradeQuantity(int buyMode, int sellMode, int buyQuantity, int sellQuantity)
    {
        return sellMode | (buyMode << 3) | ((sellQuantity << 6) & 0x1FFF) | (buyQuantity << 19);
    }
}
