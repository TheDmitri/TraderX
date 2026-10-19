// Resolves a TraderPlus V1 config file across the locations a server admin might have it,
// decides whether the TraderPlus import may run, and archives consumed legacy files.
//
// A stock TraderPlus install writes  $profile:TraderPlus\TraderPlusConfig\TraderPlusXxxConfig.json
// Early TraderX converters expected  $profile:TraderX\TraderXConfig\TraderXXxxConfig.json
// Both are accepted; the real TraderPlus name/location is the important one for painless migration.
//
// The import is a one-time bootstrap of an EMPTY TraderX install. It never runs over an existing
// TraderX configuration (a server updating TraderX that still has its old TraderPlus folder would
// otherwise get its traders, currencies and catalog overwritten on every boot).
class TraderXLegacyConfigPath
{
    static const string MIGRATION_KEY = "traderplus";

    // Returns the first existing path among the candidates, or empty string if none exist.
    //   legacyName     = historical TraderX-prefixed name (back-compat with the early converter)
    //   traderPlusName = the real filename a stock TraderPlus V1 install produces
    static string Resolve(string legacyName, string traderPlusName)
    {
        string candidate;

        candidate = TRADERX_CONFIG_DIR_SERVER + legacyName;
        if (FileExist(candidate))
            return candidate;

        candidate = TRADERX_CONFIG_DIR_SERVER + traderPlusName;
        if (FileExist(candidate))
            return candidate;

        candidate = TRADERPLUS_V1_CONFIG_DIR + traderPlusName;
        if (FileExist(candidate))
            return candidate;

        return string.Empty;
    }

    static bool HasAnySource()
    {
        if (Resolve("TraderXPriceConfig.json", "TraderPlusPriceConfig.json") != string.Empty)
            return true;
        if (Resolve("TraderXGeneralConfig.json", "TraderPlusGeneralConfig.json") != string.Empty)
            return true;
        return Resolve("TraderXVehiclesConfig.json", "TraderPlusVehiclesConfig.json") != string.Empty;
    }

    // True only for a fresh TraderX install that has TraderPlus V1 files to import.
    static bool ShouldImport()
    {
        // Called once per boot: also retries TraderPlus player data a previous import could not list.
        ConvertTraderPlusPlayerData.RetryPending();

        if (!HasAnySource())
            return false;

        if (TraderXExternalMigration.IsDone(MIGRATION_KEY))
            return false;

        if (FileExist(TRADERX_CONFIG_FILE))
        {
            GetTraderXLogger().LogInfo("[TraderX Migration] TraderPlus V1 config found but TraderX is already configured (" + TRADERX_CONFIG_FILE + ") - skipping the import. Remove that file to import TraderPlus into an empty TraderX.");
            return false;
        }

        return true;
    }

    static void MarkImported()
    {
        TraderXExternalMigration.MarkDone(MIGRATION_KEY, "TraderPlus V1 config imported. Delete this file and TraderXGeneralSettings.json to import again.");
    }

    // MakeDirectory does not create parents: on a fresh install every save below would fail
    // silently unless the whole tree exists first.
    static void EnsureTraderXDirs()
    {
        array<string> dirs = { TRADERX_CONFIG_ROOT_SERVER, TRADERX_CONFIG_DIR_SERVER, TRADERX_CATEGORIES_DIR, TRADERX_PRODUCTS_DIR, TRADERX_DB_DIR_SERVER };
        foreach (string dir : dirs)
        {
            if (!FileExist(dir))
                MakeDirectory(dir);
        }
    }

    // Takes a consumed legacy file out of the way so the import never re-triggers.
    // Files in the TraderPlus folder are left untouched: TraderPlus may still be loaded on the
    // server, and the import sentinel already prevents a re-run. Early TraderX-named files are
    // moved into TraderXConfig/deleted/, and only deleted once the copy is verified.
    static void Archive(string sourcePath, string archiveName)
    {
        if (sourcePath == string.Empty || !FileExist(sourcePath))
            return;

        if (sourcePath.IndexOf(TRADERPLUS_V1_CONFIG_DIR) == 0)
        {
            GetTraderXLogger().LogInfo("[TraderX Migration] Imported " + sourcePath + " (left in place)");
            return;
        }

        string deletedDir = TRADERX_CONFIG_DIR_SERVER + "deleted/";
        if (!FileExist(deletedDir))
            MakeDirectory(deletedDir);

        string destPath = deletedDir + archiveName;
        CopyFile(sourcePath, destPath);
        if (!FileExist(destPath))
        {
            GetTraderXLogger().LogError("[TraderX Migration] Could not archive " + sourcePath + " to " + destPath + " - keeping the original");
            return;
        }

        DeleteFile(sourcePath);
        GetTraderXLogger().LogInfo("[TraderX Migration] Archived consumed legacy config: " + sourcePath + " -> " + destPath);
    }
}
