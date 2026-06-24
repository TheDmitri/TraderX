// TraderXCarLock standalone logger -> $profile:TraderX\TraderXCarLock\Logs\TXCL_<ts>.log.
// Same API as DZLog; call sites use GetTXCLLog(). Independent of the DayZular logger.

class TXCLLogger
{
    private static ref TXCLLogger s_Instance;
    private FileHandle m_Handle;
    private bool m_Opened;

    static TXCLLogger GetInstance()
    {
        if (!s_Instance)
            s_Instance = new TXCLLogger();
        return s_Instance;
    }

    private void EnsureOpen()
    {
        if (m_Opened)
            return;
        m_Opened = true;

        if (!FileExist(TXCL_PROFILE_ROOT))
            MakeDirectory(TXCL_PROFILE_ROOT);
        if (!FileExist(TXCL_CONFIG_ROOT))
            MakeDirectory(TXCL_CONFIG_ROOT);
        if (!FileExist(TXCL_LOG_DIR))
            MakeDirectory(TXCL_LOG_DIR);

        m_Handle = OpenFile(string.Format(TXCL_LOG_FILE, FileStamp()), FileMode.WRITE);
        if (m_Handle != 0)
            FPrintln(m_Handle, "TraderXCarLock log - created " + FileStamp());
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

static TXCLLogger GetTXCLLog()
{
    return TXCLLogger.GetInstance();
}
