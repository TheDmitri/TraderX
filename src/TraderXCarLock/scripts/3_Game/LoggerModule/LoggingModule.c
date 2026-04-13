static ref TXCarLockLoggingModule g_TXCarLockLogger;

TXCarLockLoggingModule GetTXCarLockLogger()
{
    return g_TXCarLockLogger;
}

class TXCarLockLoggingModule
{
    private int m_LogLevel;
    ref TXCarLockLoggingSettings m_Settings;
    FileHandle m_FileHandle;
    float m_DeltaAccum = 0;

    void TXCarLockLoggingModule()
    {
        m_LogLevel = 0;
    }

    void Initialize()
    {
        m_FileHandle = CreateNewLogFile();

        if (GetGame().IsServer())
        {
            m_Settings = TXCarLockLoggingSettings.Load();
            m_LogLevel = m_Settings.logLevel;
        }
    }

    void OnUpdate(float deltaTime)
    {
        if (!GetGame().IsServer() || !m_Settings)
            return;

        m_DeltaAccum += deltaTime;
        if (m_DeltaAccum >= m_Settings.refreshRateInSeconds)
        {
            m_DeltaAccum = 0;
            m_Settings = TXCarLockLoggingSettings.Load();
            m_LogLevel = m_Settings.logLevel;
        }
    }

    void SetLogLevel(int logLevel)
    {
        m_LogLevel = logLevel;
    }

    void MakeDirectoryIfNotExists()
    {
        if (!FileExist(TX_CARLOCK_CONFIG_ROOT))
            MakeDirectory(TX_CARLOCK_CONFIG_ROOT);

        if (!FileExist(TX_CARLOCK_LOGGER_ROOT))
            MakeDirectory(TX_CARLOCK_LOGGER_ROOT);

        if (!FileExist(TX_CARLOCK_LOGGER_LOG_DIR))
            MakeDirectory(TX_CARLOCK_LOGGER_LOG_DIR);

        if (!FileExist(TX_CARLOCK_LOGGER_CONFIG_DIR))
            MakeDirectory(TX_CARLOCK_LOGGER_CONFIG_DIR);
    }

    string GenerateShortDateString()
    {
        int year, month, day;
        GetYearMonthDay(year, month, day);
        return "" + year + "_" + month + "_" + day;
    }

    string GenerateShortTimeString()
    {
        int hour, minute, second;
        GetHourMinuteSecond(hour, minute, second);
        return "" + hour + "_" + minute + "_" + second;
    }

    string GenerateFullTimestamp()
    {
        return GenerateShortDateString() + "-" + GenerateShortTimeString();
    }

    FileHandle CreateNewLogFile()
    {
        MakeDirectoryIfNotExists();

        string filePath = string.Format(TX_CARLOCK_LOGGER_LOG_FILE, GenerateFullTimestamp());

        m_FileHandle = OpenFile(filePath, FileMode.WRITE);

        if (m_FileHandle != 0)
        {
            FPrintln(m_FileHandle, "Creation Time: " + GenerateFullTimestamp());
            return m_FileHandle;
        }

        return null;
    }

    void Log(string content, TXCarLockLogLevel logLevel)
    {
        if (logLevel < m_LogLevel)
            return;

        string timestamp = GenerateShortTimeString();
        FPrintln(m_FileHandle, timestamp + " | " + GetLogLevelString(logLevel) + " | " + content);
    }

    void LogInfo(string content)
    {
        Log(content, TXCarLockLogLevel.Info);
    }

    void LogWarning(string content)
    {
        Log(content, TXCarLockLogLevel.Warn);
    }

    void LogError(string content)
    {
        Log(content, TXCarLockLogLevel.Error);
    }

    void LogDebug(string content)
    {
        Log(content, TXCarLockLogLevel.Debug);
    }

    string GetLogLevelString(TXCarLockLogLevel logLevel)
    {
        switch (logLevel)
        {
            case TXCarLockLogLevel.Debug:
                return "DEBUG";
            case TXCarLockLogLevel.Info:
                return "INFO";
            case TXCarLockLogLevel.Warn:
                return "WARNING";
            case TXCarLockLogLevel.Error:
                return "ERROR";
            default:
                return "";
        }

        return "";
    }
}
