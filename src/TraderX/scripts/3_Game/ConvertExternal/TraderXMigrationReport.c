// Human-readable record of a config migration: what was imported, and every source setting that
// TraderX could not carry over (no equivalent feature) or had to approximate. Written next to the
// TraderX config as MigrationReport_<source>.txt so nothing is ever dropped silently.
class TraderXMigrationReport
{
    protected static ref map<string, ref array<string>> s_Lines = new map<string, ref array<string>>();

    static void Info(string source, string line)
    {
        Add(source, "INFO    " + line);
    }

    // Converted, but the behaviour is not identical.
    static void Approx(string source, string line)
    {
        Add(source, "APPROX  " + line);
        GetTraderXLogger().LogWarning("[TraderX Migration] " + source + ": " + line);
    }

    // Not converted: TraderX has no equivalent.
    static void Skipped(string source, string line)
    {
        Add(source, "SKIPPED " + line);
        GetTraderXLogger().LogWarning("[TraderX Migration] " + source + ": not migrated - " + line);
    }

    // Step aborted (e.g. an existing TraderX file could not be read); the migration retries next boot.
    static void Error(string source, string line)
    {
        Add(source, "ERROR   " + line);
        GetTraderXLogger().LogError("[TraderX Migration] " + source + ": " + line);
    }

    protected static void Add(string source, string line)
    {
        if (!s_Lines.Contains(source))
            s_Lines.Insert(source, new array<string>());
        s_Lines.Get(source).Insert(line);
    }

    // append = add to the report of an earlier run (e.g. a retried step) instead of replacing it.
    static void Write(string source, bool append = false)
    {
        if (!s_Lines.Contains(source))
            return;

        if (!FileExist(TRADERX_CONFIG_ROOT_SERVER))
            MakeDirectory(TRADERX_CONFIG_ROOT_SERVER);
        if (!FileExist(TRADERX_CONFIG_DIR_SERVER))
            MakeDirectory(TRADERX_CONFIG_DIR_SERVER);

        string path = TRADERX_CONFIG_DIR_SERVER + "MigrationReport_" + source + ".txt";
        FileMode mode = FileMode.WRITE;
        if (append && FileExist(path))
            mode = FileMode.APPEND;
        FileHandle fh = OpenFile(path, mode);
        if (!fh)
            return;

        if (mode == FileMode.APPEND)
            FPrintln(fh, "");
        FPrintln(fh, "TraderX migration report - " + source);
        FPrintln(fh, "INFO = imported as-is, APPROX = imported with a behaviour difference, SKIPPED = no TraderX equivalent, ERROR = step aborted (fix and restart to retry)");
        FPrintln(fh, "");
        foreach (string line : s_Lines.Get(source))
        {
            FPrintln(fh, line);
        }
        CloseFile(fh);
    }
}
