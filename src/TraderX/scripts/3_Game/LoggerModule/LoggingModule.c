// Global accessor for the logger service (resolved from the DayZular registry).
// Self-registers on first use so early callers (services that log during their own
// construction) work regardless of registration order during the CF->DayZular migration.
static ref TraderXLoggingModule g_TraderXLogger;

static TraderXLoggingModule GetTraderXLogger()
{
    if (!g_TraderXLogger)
    {
        g_TraderXLogger = TraderXLoggingModule.Cast(DZlrServiceRegistry.GetInstance().GetService(TraderXLoggingModule));
        if (!g_TraderXLogger)
        {
            g_TraderXLogger = new TraderXLoggingModule();
            DZlrServiceRegistry.GetInstance().RegisterService(TraderXLoggingModule, g_TraderXLogger);
        }
    }
    return g_TraderXLogger;
}

class TraderXLoggingModule : DayZularService
{
    private int networkSync_LogLevel;

    ref TraderXLoggingSettings settings;

    FileHandle fileHandle;

    float dtime = 0;

    void TraderXLoggingModule()
    {
        g_TraderXLogger = this;

        // Subscribe to DayZular lifecycle events (replaces CF EnableMissionStart/EnableUpdate).
        GetDayZularEvents().OnMissionStart.Insert(OnMissionStart);
        GetDayZularEvents().OnUpdate.Insert(OnUpdate);

        // Clients receive the server's log level over RPC.
        if (GetGame().IsClient())
            GetDayZularRPCManager().AddRPC("TraderX", "GetLogLevelResponse", this, DayZularRPCType.Client);
    }

    void OnMissionStart()
    {
        fileHandle = CreateNewLogFile();

        if(GetGame().IsServer())
        {
            settings = TraderXLoggingSettings.Load();

            networkSync_LogLevel = settings.logLevel;
            SynchLogLevel();
        }
    }

    void GetLogLevelResponse(PlayerIdentity sender, ParamsReadContext ctx)
    {
        Param1<int> data;
        if (!ctx.Read(data))
            return;

        networkSync_LogLevel = data.param1;
    }

    void SynchLogLevel()
    {
        GetDayZularRPCManager().SendRPC("TraderX", "GetLogLevelResponse", new Param1<int>(networkSync_LogLevel), true, NULL);
    }

    void OnUpdate(float deltaTime)
    {
        if(!GetGame().IsServer())
            return;

        if(!settings)
            return;

        dtime += deltaTime;
        if(dtime >= settings.refreshRateInSeconds)
        {
            dtime = 0;

            settings = TraderXLoggingSettings.Load();

            networkSync_LogLevel = settings.logLevel;
            SynchLogLevel();
        }
    }


    void MakeDirectoryIfNotExists()
    {
        if(!FileExist(TRADERX_CONFIG_ROOT_SERVER))
            MakeDirectory(TRADERX_CONFIG_ROOT_SERVER);

        if(!FileExist(TRADERX_LOG_FOLDER))
            MakeDirectory(TRADERX_LOG_FOLDER);

        if(!FileExist(TRADERX_LOGGER_CONFIG_DIR))
            MakeDirectory(TRADERX_LOGGER_CONFIG_DIR);

        if(!FileExist(TRADERX_LOGGER_LOG_DIR))
            MakeDirectory(TRADERX_LOGGER_LOG_DIR);
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
        string dateStr = GenerateShortDateString();
        string timeStr = GenerateShortTimeString();

        return dateStr + "-" + timeStr;
    }

    FileHandle CreateNewLogFile()
    {
        MakeDirectoryIfNotExists();

        string filePath = string.Format(TRADERX_LOGGER_LOG_FILE, GenerateFullTimestamp());

        fileHandle = OpenFile(filePath, FileMode.WRITE);

        if(fileHandle != 0)
        {
            FPrintln(fileHandle, "Creation Time: " + GenerateFullTimestamp());
            return fileHandle;
        }

        return null;
    }

    // Cheap level check so hot paths can skip building an expensive string.Format
    // argument that Log() would just discard. Mirrors the filter in Log().
    bool ShouldLog(TraderXLogLevel logLevel)
    {
        return logLevel >= networkSync_LogLevel;
    }

    void Log(string content, TraderXLogLevel logLevel)
    {
        if(logLevel < networkSync_LogLevel)
            return;

        string timestamp = GenerateShortTimeString();
        FPrintln(fileHandle, timestamp + " | " + GetLogLevelString(logLevel) + " | " + content);
    }

    void LogInfo(string content)
    {
        Log(content, TraderXLogLevel.Info);
    }

    void LogWarning(string content)
    {
        Log(content, TraderXLogLevel.Warn);
    }

    void LogError(string content)
    {
        Log(content, TraderXLogLevel.Error);
    }

    void LogDebug(string content)
    {
        Log(content, TraderXLogLevel.Debug);
    }

    string GetLogLevelString(TraderXLogLevel logLevel)
    {
        switch(logLevel)
        {
             case TraderXLogLevel.Debug:
                return "DEBUG";
            case TraderXLogLevel.Info:
                return "INFO";
            case TraderXLogLevel.Warn:
                return "WARNING";
            case TraderXLogLevel.Error:
                return "ERROR";
            default:
                return "";
        }

        return "";
    }

}
