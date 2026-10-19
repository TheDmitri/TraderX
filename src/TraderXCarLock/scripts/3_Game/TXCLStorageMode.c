// How vehicle lock data is persisted.
//
// Lock data lives in CF ModStorage under the "TraderXCarLock" mod (storageVersion in config.cpp):
// CF keeps each mod's data in its own sized block, so vehicles saved without TraderXCarLock (e.g. by
// TraderX 1.x) load cleanly and other mods' data can never be misread.
//
// Earlier TraderXCarLock builds wrote the lock as a raw Param4 straight into the vehicle save. On a
// server that ran such a build, vehicles that have no ModStorage block yet still carry that raw data
// and it must be consumed once, or every mod reading after it would be misaligned. Whether this
// server is such a server is decided ONCE, on the first boot of this build, before the world is
// loaded and before TXCLSettings.json is (re)created, then remembered in storage_mode.txt.
class TXCLStorageMode
{
    static const string MODE_FILE = TXCL_CONFIG_ROOT + "storage_mode.txt";
    static const string MODE_LEGACY = "legacy";
    static const string MODE_CLEAN = "clean";

    protected static bool s_Resolved;
    protected static bool s_ReadsLegacyData;

    // Call as early as possible on the server (module init), before vehicles are loaded.
    static void Resolve()
    {
        if (s_Resolved)
            return;
        s_Resolved = true;

        string mode = ReadMode();
        if (mode == string.Empty)
        {
            // TXCLSettings.json only exists if an earlier TraderXCarLock build already ran here.
            mode = MODE_CLEAN;
            if (FileExist(TXCL_CONFIG_FILENAME))
                mode = MODE_LEGACY;
            WriteMode(mode);
        }

        s_ReadsLegacyData = (mode == MODE_LEGACY);
    }

    static bool ReadsLegacyData()
    {
        Resolve();
        return s_ReadsLegacyData;
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
        return line;
    }

    protected static void WriteMode(string mode)
    {
        if (!FileExist(TXCL_PROFILE_ROOT))
            MakeDirectory(TXCL_PROFILE_ROOT);
        if (!FileExist(TXCL_CONFIG_ROOT))
            MakeDirectory(TXCL_CONFIG_ROOT);

        FileHandle fh = OpenFile(MODE_FILE, FileMode.WRITE);
        if (!fh)
            return;

        FPrintln(fh, mode);
        CloseFile(fh);
    }
}
