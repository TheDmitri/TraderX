// TraderXGarage standalone logger -> $profile:TraderX/TraderXGarage/Logs/TXG_<ts>.log.
// Same API as DZLog; call sites use GetTXGLog(). Independent of the DayZular logger.

class TXGLogger
{
    private static ref TXGLogger s_Instance;
    private FileHandle m_Handle;
    private bool m_Opened;

    static TXGLogger GetInstance()
    {
        if (!s_Instance)
            s_Instance = new TXGLogger();
        return s_Instance;
    }

    private void EnsureOpen()
    {
        if (m_Opened)
            return;
        m_Opened = true;

        if (!FileExist(TXGConstants.ROOT_DIR))
            MakeDirectory(TXGConstants.ROOT_DIR);
        if (!FileExist(TXGConstants.CONFIG_DIR))
            MakeDirectory(TXGConstants.CONFIG_DIR);
        if (!FileExist(TXGConstants.LOG_DIR))
            MakeDirectory(TXGConstants.LOG_DIR);

        m_Handle = OpenFile(string.Format(TXGConstants.LOG_FILE, FileStamp()), FileMode.WRITE);
        if (m_Handle != 0)
            FPrintln(m_Handle, "TraderXGarage log - created " + FileStamp());
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

static TXGLogger GetTXGLog()
{
    return TXGLogger.GetInstance();
}
