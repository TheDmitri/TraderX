// TraderXBanking standalone logger. Writes to $profile:TraderX/TraderXBanking/Logs/TXB_<ts>.log,
// one file per session. Same API (LogInfo/LogWarning/LogError/LogDebug) as the old shared
// DayZular logger, so call sites only changed the accessor name. Fully independent of DayZular.

class TXBLogger
{
    private static ref TXBLogger s_Instance;
    private FileHandle m_Handle;
    private bool m_Opened;

    static TXBLogger GetInstance()
    {
        if (!s_Instance)
            s_Instance = new TXBLogger();
        return s_Instance;
    }

    private void EnsureOpen()
    {
        if (m_Opened)
            return;
        m_Opened = true;

        if (!FileExist(TXBConstants.ROOT_DIR))
            MakeDirectory(TXBConstants.ROOT_DIR);
        if (!FileExist(TXBConstants.CONFIG_DIR))
            MakeDirectory(TXBConstants.CONFIG_DIR);
        if (!FileExist(TXBConstants.LOG_DIR))
            MakeDirectory(TXBConstants.LOG_DIR);

        m_Handle = OpenFile(string.Format(TXBConstants.LOG_FILE, FileStamp()), FileMode.WRITE);
        if (m_Handle != 0)
            FPrintln(m_Handle, "TraderXBanking log - created " + FileStamp());
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

static TXBLogger GetTXBLog()
{
    return TXBLogger.GetInstance();
}
