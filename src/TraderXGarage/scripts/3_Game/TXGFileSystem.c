/**
 * TXGFileSystem (copy of TraderXFileSystem; TraderXGarage does not depend on TraderX)
 * FindFile wrapper that works on DayZ 1.29 and 1.30.
 *
 * DayZ 1.30 FindFile no longer resolves the "$profile:" prefix. On 1.30 final
 * it only lists paths relative to the server working directory, written with
 * forward slashes ("profiles/TraderX/x/*.json"); "$profile:", "..", "./",
 * backslashes and (since 1.30 final) absolute paths all return nothing.
 * 1.29 is the opposite: only "$profile:" works.
 *
 * Find() rewrites "$profile:" to the -profiles directory and tries that first,
 * then falls back to the original pattern. A server started with a relative
 * -profiles (the usual "-profiles=profiles") is fully covered; one whose
 * profile lives outside the server directory cannot be listed on 1.30.
 */
class TXGFileSystem
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
        }

        return FindFile(pattern, fileName, fileAttributes, flags);
    }

    /**
     * Replaces "$profile:" with the -profiles directory from the command line,
     * using forward slashes only. Returns false when the path has no "$profile:"
     * prefix or no -profiles was given (e.g. on a client), in which case the
     * caller keeps the original path.
     */
    static bool ResolveProfilePath(string path, out string resolved)
    {
        if (path.IndexOf(PROFILE_PREFIX) != 0)
            return false;

        string profileDir;
        if (!GetCLIParam("profiles", profileDir) || profileDir == string.Empty)
            return false;

        profileDir.Replace("\"", "");
        profileDir.Replace("\\", "/");
        while (profileDir.IndexOf("./") == 0)
            profileDir = profileDir.Substring(2, profileDir.Length() - 2);
        while (profileDir.Length() > 0 && profileDir.Get(profileDir.Length() - 1) == "/")
            profileDir = profileDir.Substring(0, profileDir.Length() - 1);

        string rest = path.Substring(PROFILE_PREFIX.Length(), path.Length() - PROFILE_PREFIX.Length());
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
