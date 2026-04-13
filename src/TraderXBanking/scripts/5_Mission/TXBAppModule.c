// TraderXBanking - App Module (Service Registration)
// Subscribes to DayZular's service registration event to register all TraderXBanking services

class TXBAppInit
{
	void TXBAppInit()
	{
		AppModule.GetOnRegisterServices().Insert(RegisterServices);
	}

	static void RegisterServices()
	{
		auto registry = DZlrServiceRegistry.GetInstance();

		DZLog().LogInfo("[TXB] Registering TraderXBanking services...");

		// Config service first (no dependencies)
		registry.RegisterService(TXBConfigService, new TXBConfigService());

		// Banking service (depends on config)
		registry.RegisterService(TXBBankingService, new TXBBankingService());

		// RPC service (depends on config + banking)
		registry.RegisterService(TXBRPCService, new TXBRPCService());

		// UI service (client-only, depends on banking)
		registry.RegisterService(TXBUIService, new TXBUIService());

		// Register as TraderX payment provider (enables bank payments in trader)
		TraderXProviderRegistry.GetInstance().RegisterBankingProvider(new TXBPaymentProvider());

		DZLog().LogInfo("[TXB] TraderXBanking services registered");
	}
}

// Auto-initialize on script load
ref TXBAppInit g_TXBAppInit = new TXBAppInit();
