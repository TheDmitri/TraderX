// TraderXSafeZone standalone logger -> $profile:TraderX/TraderXSafeZone/Logs/TXSZ_<ts>.log.
// Same API as DZLog; call sites use GetTXSZLog(). Independent of the DayZular logger.

class TXSZLogger
{
    private static ref TXSZLogger s_Instance;
    private FileHandle m_Handle;
    private bool m_Opened;

    static TXSZLogger GetInstance()
    {
        if (!s_Instance)
            s_Instance = new TXSZLogger();
        return s_Instance;
    }

    private void EnsureOpen()
    {
        if (m_Opened)
            return;
        m_Opened = true;

        if (!FileExist(Safe_Zone_Root_Server))
            MakeDirectory(Safe_Zone_Root_Server);
        if (!FileExist(Safe_Zone_Root_Server + "TraderXSafeZone/"))
            MakeDirectory(Safe_Zone_Root_Server + "TraderXSafeZone/");
        if (!FileExist(Safe_Zone_Log_Dir))
            MakeDirectory(Safe_Zone_Log_Dir);

        m_Handle = OpenFile(string.Format(Safe_Zone_Log_File, FileStamp()), FileMode.WRITE);
        if (m_Handle != 0)
            FPrintln(m_Handle, "TraderXSafeZone log - created " + FileStamp());
    }

    private string Pad2(int v)
    {
        if (v < 10)
            return "0" + v;
        return "" + v;
    }

    private string FileStamp()
    {
        int y, mo, d, h, mi, s;
        GetYearMonthDay(y, mo, d);
        GetHourMinuteSecond(h, mi, s);
        return "" + y + "_" + Pad2(mo) + "_" + Pad2(d) + "-" + Pad2(h) + "_" + Pad2(mi) + "_" + Pad2(s);
    }

    private string TimeStamp()
    {
        int h, mi, s;
        GetHourMinuteSecond(h, mi, s);
        return Pad2(h) + ":" + Pad2(mi) + ":" + Pad2(s);
    }

    void Write(string level, string message)
    {
        EnsureOpen();
        if (m_Handle != 0)
            FPrintln(m_Handle, TimeStamp() + " | " + level + " | " + message);
    }

    void LogInfo(string message)    { Write("INFO", message); }
    void LogWarning(string message) { Write("WARN", message); }
    void LogError(string message)   { Write("ERROR", message); }
    void LogDebug(string message)   { Write("DEBUG", message); }
}

static TXSZLogger GetTXSZLog()
{
    return TXSZLogger.GetInstance();
}
