// TraderXGarage - Module Initialization
// Auto-initializes on mod load. Registers all services with DayZular's service registry.

class TraderXGarageInit
{
	void TraderXGarageInit()
	{
		AppModule.GetOnRegisterServices().Insert(RegisterServices);
	}

	static void RegisterServices()
	{
		auto registry = DZlrServiceRegistry.GetInstance();

		// Server-side garage logic
		registry.RegisterService(GarageService, new GarageService());

		// RPC routing (both server and client)
		registry.RegisterService(GarageRpcService, new GarageRpcService());

		// Client-side UI management
		registry.RegisterService(GarageUIService, new GarageUIService());

		// Register as TraderX garage provider (enables garage tab in trader menu)
		TraderXProviderRegistry.GetInstance().RegisterGarageProvider(new TXGGarageProvider());
	}
}

// Global auto-init instance — triggers registration when the mod loads
ref TraderXGarageInit g_TraderXGarageInit = new TraderXGarageInit();
