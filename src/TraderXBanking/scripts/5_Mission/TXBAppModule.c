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

		GetTXBLog().LogInfo("[TXB] Registering TraderXBanking services...");

		// Config service first (no dependencies)
		registry.RegisterService(TXBConfigService, new TXBConfigService());

		// Banking service (depends on config)
		registry.RegisterService(TXBBankingService, new TXBBankingService());

		// RPC service (depends on config + banking)
		registry.RegisterService(TXBRPCService, new TXBRPCService());

		// Vault service (server: item storage; client: cache)
		registry.RegisterService(TXBVaultService, new TXBVaultService());

		// Vault RPC service
		registry.RegisterService(TXBVaultRPCService, new TXBVaultRPCService());

		// UI service (client-only, depends on banking)
		registry.RegisterService(TXBUIService, new TXBUIService());

		// Bank NPC service - spawns bank NPCs in standalone mode
		// (when TraderX is present, it manages NPCs instead)
		registry.RegisterService(TXBBankNpcService, new TXBBankNpcService());

		// Register as TraderX payment provider (enables bank payments in trader)
		// Only when TraderX mod is present
#ifdef TraderX
		TraderXProviderRegistry.GetInstance().RegisterBankingProvider(new TXBPaymentProvider());
#endif

		GetTXBLog().LogInfo("[TXB] TraderXBanking services registered");
	}
}

// Auto-initialize on script load
ref TXBAppInit g_TXBAppInit = new TXBAppInit();
