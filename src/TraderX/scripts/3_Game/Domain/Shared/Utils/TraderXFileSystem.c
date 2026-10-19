/**
 * TraderXFileSystem
 * FindFile wrapper that works on DayZ 1.29 and 1.30.
 *
 * DayZ 1.30 FindFile no longer resolves the "$profile:" prefix. On 1.30 final
 * it only lists paths relative to the server working directory, written with
 * forward slashes ("profiles/TraderX/x/*.json"); "$profile:", "..", "./",
 * backslashes and (since 1.30 final) absolute paths all return nothing.
 * 1.29 is the opposite: only "$profile:" works.
 *
 * Find() rewrites "$profile:" to the -profiles directory ("$mission:" to the mission
 * folder) and tries that first,
 * then falls back to the original pattern. A server started with a relative
 * -profiles (the usual "-profiles=profiles") is fully covered; one whose
 * profile lives outside the server directory cannot be listed on 1.30.
 */
class TraderXFileSystem
{
    static const string PROFILE_PREFIX = "$profile:";

    static FindFileHandle Find(string pattern, out string fileName, out FileAttr fileAttributes, FindFileFlags flags)
    {
        string profilePattern;
        if (ResolveProfilePath(pattern, profilePattern))
        {
            FindFileHandle handle = FindFile(profilePattern, fileName, fileAttributes, flags);
            if (handle)
                return handle;

            string basePattern;
            if (ResolveProfileBasenamePath(pattern, basePattern))
            {
                FindFileHandle baseHandle = FindFile(basePattern, fileName, fileAttributes, flags);
                if (baseHandle)
                    return baseHandle;
            }
        }

        return FindFile(pattern, fileName, fileAttributes, flags);
    }

    static bool ResolveProfileBasenamePath(string path, out string resolved)
    {
        if (path.IndexOf(PROFILE_PREFIX) != 0)
            return false;
        string profileDir;
        if (!GetCLIParam("profiles", profileDir) || profileDir == string.Empty)
            return false;
        profileDir.Replace("\"", "");
        profileDir.Replace("\\", "/");
        while (profileDir.Length() > 0 && profileDir.Get(profileDir.Length() - 1) == "/")
            profileDir = profileDir.Substring(0, profileDir.Length() - 1);
        int lastSlash = -1;
        for (int i = 0; i < profileDir.Length(); i++)
        {
            if (profileDir.Get(i) == "/")
                lastSlash = i;
        }
        if (lastSlash < 0 || lastSlash + 1 >= profileDir.Length())
            return false;
        string baseName = profileDir.Substring(lastSlash + 1, profileDir.Length() - lastSlash - 1);
        string rest = path.Substring(PROFILE_PREFIX.Length(), path.Length() - PROFILE_PREFIX.Length());
        rest.Replace("\\", "/");
        while (rest.Length() > 0 && rest.Get(0) == "/")
            rest = rest.Substring(1, rest.Length() - 1);
        resolved = baseName + "/" + rest;
        return true;
    }

    /**
     * Replaces "$profile:" with the -profiles directory from the command line,
     * using forward slashes only. Returns false when the path has no "$profile:"
     * prefix or no -profiles was given (e.g. on a client), in which case the
     * caller keeps the original path.
     */
    static const string MISSION_PREFIX = "$mission:";
    protected static string s_MissionFolder;

    // Mission folder relative to the server directory ("mpmissions/<template>"). The engine's
    // mission path is not set yet when mods initialise, so fall back to the server config template.
    static string GetMissionFolder()
    {
        if (s_MissionFolder != string.Empty)
            return s_MissionFolder;

        if (GetDayZGame())
        {
            string folder = GetDayZGame().GetMissionFolderPath();
            folder.Replace("$CurrentDir:", "");
            folder.Replace("\\", "/");
            if (folder != string.Empty)
            {
                s_MissionFolder = folder;
                return s_MissionFolder;
            }
        }

        string configFile;
        if (!GetCLIParam("config", configFile) || configFile == string.Empty)
            configFile = "serverDZ.cfg";

        FileHandle fh = OpenFile("$CurrentDir:" + configFile, FileMode.READ);
        if (!fh)
            return string.Empty;

        string line;
        while (FGets(fh, line) >= 0)
        {
            int templateAt = line.IndexOf("template");
            int commentAt = line.IndexOf("//");
            if (templateAt < 0 || (commentAt >= 0 && commentAt < templateAt))
                continue;
            int quoteOpen = line.IndexOfFrom(templateAt, "\"");
            if (quoteOpen < 0)
                continue;
            int quoteClose = line.IndexOfFrom(quoteOpen + 1, "\"");
            if (quoteClose <= quoteOpen)
                continue;
            s_MissionFolder = "mpmissions/" + line.Substring(quoteOpen + 1, quoteClose - quoteOpen - 1);
            break;
        }
        CloseFile(fh);
        return s_MissionFolder;
    }

    static bool ResolveProfilePath(string path, out string resolved)
    {
        string prefix;
        string profileDir;
        if (path.IndexOf(PROFILE_PREFIX) == 0)
        {
            prefix = PROFILE_PREFIX;
            if (!GetCLIParam("profiles", profileDir) || profileDir == string.Empty)
                return false;
        }
        else if (path.IndexOf(MISSION_PREFIX) == 0)
        {
            // Mission folder as set by the engine, e.g. "$CurrentDir:mpmissions\dayzOffline.chernarusplus".
            prefix = MISSION_PREFIX;
            profileDir = GetMissionFolder();
            if (profileDir == string.Empty)
                return false;
        }
        else
        {
            return false;
        }

        profileDir.Replace("\"", "");
        profileDir.Replace("\\", "/");
        while (profileDir.IndexOf("./") == 0)
            profileDir = profileDir.Substring(2, profileDir.Length() - 2);
        while (profileDir.Length() > 0 && profileDir.Get(profileDir.Length() - 1) == "/")
            profileDir = profileDir.Substring(0, profileDir.Length() - 1);

        string rest = path.Substring(prefix.Length(), path.Length() - prefix.Length());
        rest.Replace("\\", "/");
        while (rest.Length() > 0 && rest.Get(0) == "/")
            rest = rest.Substring(1, rest.Length() - 1);

        if (profileDir == string.Empty)
            resolved = rest;
        else
            resolved = profileDir + "/" + rest;
        return true;
    }
}
