// What a vehicle save holds after CF's block when it has no TraderX ModStorage block, i.e. a vehicle
// never saved by this build:
//   traderx    - an earlier TraderX ran on this server: its raw Param1<int> vehicle id
//   traderplus - a TraderPlus V1 world: TraderPlus's Param1<int> vehicle id then TraderPlus CarLock's Param3<bool,int,int> (cars only)
//   vanilla    - TraderX was never installed: nothing
// Decided ONCE, on the first boot of this build, before the world is loaded (module OnInit), then
// remembered in world_format.txt: vehicles keep their old layout until the server saves them again.
class TraderXWorldFormat
{
    static const string MODE_FILE = TRADERX_CONFIG_DIR_SERVER + "world_format.txt";
    static const string TRADERX = "traderx";
    static const string TRADERPLUS = "traderplus";
    static const string VANILLA = "vanilla";

    protected static string s_Format;

    static void Resolve()
    {
        if (s_Format != string.Empty)
            return;

        s_Format = ReadMode();
        if (s_Format != string.Empty)
            return;

        if (FileExist(TRADERX_CONFIG_FILE))
            s_Format = TRADERX;
        else if (TraderXLegacyConfigPath.HasAnySource())
            s_Format = TRADERPLUS;
        else
            s_Format = VANILLA;

        WriteMode(s_Format);
    }

    static string Get()
    {
        if (s_Format == string.Empty)
        {
            // Not resolved at module init (should not happen on a server): never guess a foreign layout
            s_Format = ReadMode();
            if (s_Format == string.Empty)
                s_Format = TRADERX;
        }
        return s_Format;
    }

    protected static string ReadMode()
    {
        if (!FileExist(MODE_FILE))
            return string.Empty;

        FileHandle fh = OpenFile(MODE_FILE, FileMode.READ);
        if (!fh)
            return string.Empty;

        string line;
        FGets(fh, line);
        CloseFile(fh);
        line.TrimInPlace();
        if (line != TRADERX && line != TRADERPLUS && line != VANILLA)
            return string.Empty;
        return line;
    }

    protected static void WriteMode(string mode)
    {
        if (!FileExist(TRADERX_CONFIG_ROOT_SERVER))
            MakeDirectory(TRADERX_CONFIG_ROOT_SERVER);
        if (!FileExist(TRADERX_CONFIG_DIR_SERVER))
            MakeDirectory(TRADERX_CONFIG_DIR_SERVER);

        FileHandle fh = OpenFile(MODE_FILE, FileMode.WRITE);
        if (!fh)
            return;

        FPrintln(fh, mode);
        CloseFile(fh);
        GetTraderXLogger().LogInfo("[TraderX] Vehicle save format of this world: " + mode);
    }
}
