
static ref TraderXLoggingModule GetTraderXLogger()
{
    return TraderXLoggingModule.Cast(CF_ModuleCoreManager.Get(TraderXLoggingModule));
}

[CF_RegisterModule(TraderXLoggingModule)]
class TraderXLoggingModule : CF_ModuleGame
{
    private int networkSync_LogLevel;

    ref TraderXLoggingSettings settings;

    FileHandle fileHandle;

    float dtime = 0;


    override void OnInit()
    {
        super.OnInit();
        
        EnableUpdate();
        EnableMissionStart();
    }

    override void OnMissionStart(Class sender, CF_EventArgs args)
    {
        super.OnMissionStart(sender, args);

        fileHandle = CreateNewLogFile();

        if(GetGame().IsServer())
        {
            settings = TraderXLoggingSettings.Load();

            networkSync_LogLevel = settings.logLevel;
            SynchLogLevel();       
        }
        else
        {
            AddLegacyRPC("GetLogLevelResponse", SingleplayerExecutionType.Client);
        }
    }

    void GetLogLevelResponse(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		 if (type != CallType.Client)
			 return;

		 Param1<int> data;
		 if (!ctx.Read(data))
            return;

		 networkSync_LogLevel = data.param1;
	}

    void SynchLogLevel()
    {
        GetRPCManager().SendRPC(ClassName(), "GetLogLevelResponse",  new Param1<int>(networkSync_LogLevel), true, NULL);
    }

    override void OnUpdate(Class sender, CF_EventArgs args)
    {
        auto update = CF_EventUpdateArgs.Cast(args);

        if(!GetGame().IsServer())
            return;

        if(!settings)
            return;

        dtime += update.DeltaTime;
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