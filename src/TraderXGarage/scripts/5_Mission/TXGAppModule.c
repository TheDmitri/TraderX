// TraderXGarage - App Module (Service Registration)
// Subscribes to DayZular's service registration event to register all garage services.

class TXGAppInit
{
    void TXGAppInit()
    {
        AppModule.GetOnRegisterServices().Insert(RegisterServices);
    }

    static void RegisterServices()
    {
        auto registry = DZlrServiceRegistry.GetInstance();

        GetTXGLog().LogInfo("[TXG] Registering TraderXGarage services...");

        // Config first (no dependencies).
        registry.RegisterService(TXGConfigService, new TXGConfigService());

        // Core garage service (depends on config).
        registry.RegisterService(TXGGarageService, new TXGGarageService());

        // RPC service (depends on config + garage).
        registry.RegisterService(TXGRPCService, new TXGRPCService());

        // UI service (client-only menu lifecycle).
        registry.RegisterService(TXGUIService, new TXGUIService());

        // NPC service — spawns garage NPCs in standalone mode.
        registry.RegisterService(TXGNpcService, new TXGNpcService());

        // Register as the TraderX garage provider (cross-mod port).
        // Only when the TraderX core (US-033 interfaces) is present.
#ifdef TraderX
        TraderXProviderRegistry.GetInstance().RegisterGarageProvider(new TXGGarageProvider());
#endif

        GetTXGLog().LogInfo("[TXG] TraderXGarage services registered");
    }
}

// Auto-initialize on script load
ref TXGAppInit g_TXGAppInit = new TXGAppInit();
