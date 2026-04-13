class TraderXCarLockAppInit
{
    void TraderXCarLockAppInit()
    {
        AppModule.GetOnRegisterServices().Insert(RegisterServices);
    }

    static void RegisterServices()
    {
        auto registry = DZlrServiceRegistry.GetInstance();

        // Register logger first (other services depend on it)
        registry.RegisterService(TXCarLockLoggingService, new TXCarLockLoggingService());

        GetTXCarLockLogger().LogInfo("Registering TraderXCarLock services...");

        // Config service (no dependencies beyond logger)
        registry.RegisterService(TXCarLockConfigService, new TXCarLockConfigService());

        // RPC service (depends on config for player join events)
        registry.RegisterService(TXCarLockRPCService, new TXCarLockRPCService());

        // UI service (client-only, no dependencies)
        registry.RegisterService(TXCarLockUIService, new TXCarLockUIService());

        GetTXCarLockLogger().LogInfo("TraderXCarLock services registered");
    }
}

ref TraderXCarLockAppInit g_TraderXCarLockAppInit = new TraderXCarLockAppInit();
