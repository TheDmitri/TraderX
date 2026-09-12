// Resolves a TraderPlus V1 config file across the locations a server admin might have it,
// and archives a consumed file so the migration never re-triggers.
//
// A stock TraderPlus install writes  $profile:TraderPlus\TraderPlusConfig\TraderPlusXxxConfig.json
// Early TraderX converters expected  $profile:TraderX\TraderXConfig\TraderXXxxConfig.json
// Both are accepted; the real TraderPlus name/location is the important one for painless migration.
class TraderXLegacyConfigPath
{
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

    // Move a consumed legacy file into TraderXConfig\deleted\ so it won't re-trigger conversion.
    // archiveName is the (known) basename to store it under.
    static void Archive(string sourcePath, string archiveName)
    {
        if (sourcePath == string.Empty || !FileExist(sourcePath))
            return;

        string deletedDir = TRADERX_CONFIG_DIR_SERVER + "deleted\\";
        if (!FileExist(deletedDir))
            MakeDirectory(deletedDir);

        string destPath = deletedDir + archiveName;
        CopyFile(sourcePath, destPath);
        DeleteFile(sourcePath);
        GetTraderXLogger().LogInfo("[TraderX Migration] Archived consumed legacy config: " + sourcePath + " -> " + destPath);
    }
}
