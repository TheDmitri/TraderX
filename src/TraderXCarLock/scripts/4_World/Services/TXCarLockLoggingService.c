static ref TXCarLockLoggingService g_TXCarLockLoggingService;

TXCarLockLoggingService GetTXCarLockLoggingService()
{
    if (!g_TXCarLockLoggingService)
        g_TXCarLockLoggingService = TXCarLockLoggingService.Cast(DZlrServiceRegistry.GetInstance().GetService(TXCarLockLoggingService));
    return g_TXCarLockLoggingService;
}

class TXCarLockLoggingService : DayZularService
{
    void TXCarLockLoggingService()
    {
        g_TXCarLockLoggingService = this;

        // Create the logger module
        g_TXCarLockLogger = new TXCarLockLoggingModule();
        g_TXCarLockLogger.Initialize();

        GetDayZularEvents().OnUpdate.Insert(OnUpdate);
    }

    void OnUpdate(float deltaTime)
    {
        if (g_TXCarLockLogger)
            g_TXCarLockLogger.OnUpdate(deltaTime);
    }
}
