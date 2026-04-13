// TraderX - DayZular Service Registration
// Subscribe to AppModule's registration event to register TraderX services

class TraderXAppInit
{
    void TraderXAppInit()
    {
        AppModule.GetOnRegisterServices().Insert(RegisterServices);
    }

    static void RegisterServices()
    {
        auto registry = DZlrServiceRegistry.GetInstance();

        // Register logger first (other services depend on it)
        registry.RegisterService(TraderXLoggingModule, new TraderXLoggingModule());

        GetTraderXLogger().LogInfo("Registering TraderX DayZular services...");

        // Register core module (handles config, RPCs, lifecycle)
        registry.RegisterService(TraderXModule, new TraderXModule());

        // Register UI service (client-only, manages ComponentMenu and trader UI)
        registry.RegisterService(TraderXUIService, new TraderXUIService());

        GetTraderXLogger().LogInfo("TraderX DayZular services registered");
    }
}

// Auto-initialize on script load
ref TraderXAppInit g_TraderXAppInit = new TraderXAppInit();
