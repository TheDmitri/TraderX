class TraderXAppInit
{
	void TraderXAppInit()
	{
		AppModule.GetOnRegisterServices().Insert(RegisterServices);
	}

	static void RegisterServices()
	{
		auto registry = DZlrServiceRegistry.GetInstance();

		// Logger first — other services log during their own construction. Self-registers
		// into the registry on this first call (replaces the old CF_ModuleGame auto-register).
		GetTraderXLogger();

		// UI lifecycle (ComponentMenu management) — first DayZular-native service
		registry.RegisterService(TraderXUIService, new TraderXUIService());

		// HUD toast notifications (client FIFO slide-toast). Replaces the legacy
		// NotificationContainer/NotificationCard MVVM stack (kept dormant).
		registry.RegisterService(TraderXToastService, new TraderXToastService());

		// Core services migrated to the registry (US-022). Their GetInstance()
		// now resolves from the registry; all existing call sites keep working.
		// Transaction (queues only) and Selection (UI cart) are ctor-safe to build
		// at bootstrap.
		registry.RegisterService(TraderXTransactionService, new TraderXTransactionService());
		registry.RegisterService(TraderXSelectionService, new TraderXSelectionService());
		// NOTE: TraderXCurrencyService is intentionally NOT registered here — its
		// server ctor loads currency config and must run after
		// TraderXConfigurationService.Initialize(); it lazy-registers on first use.

		// Other services will be registered here as they extend DayZularService during migration.
		// Existing services remain accessible via their GetInstance() singletons until migrated.
	}
}

ref TraderXAppInit g_TraderXAppInit = new TraderXAppInit();
