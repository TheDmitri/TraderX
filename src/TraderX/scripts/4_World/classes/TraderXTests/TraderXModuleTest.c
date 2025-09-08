
static ref TraderXModuleTest GetTraderXModuleTest()
{
    return TraderXModuleTest.Cast(CF_ModuleCoreManager.Get(TraderXModuleTest));
}

[CF_RegisterModule(TraderXModuleTest)]
class TraderXModuleTest: CF_ModuleWorld
{
    ref TraderXDebugSettings debugSettings;

    override void OnInit()
	{
        super.OnInit();
        EnableMissionStart();
    }

    void StartUnitTest()
    {
        GetTraderXLogger().LogInfo("[TEST RUNNER] Starting TraderX test suite...");
        
        // Load debug settings
        TraderXDebugSettings debugSettings = TraderXDebugSettingsRepository.Load();
        
        // Log settings summary
        GetTraderXLogger().LogInfo(debugSettings.GetSettingsSummary());
        
        // Enable debug mode if configured
        if (debugSettings.enableDebugMode)
        {
            GetTraderXLogger().LogInfo("[TEST RUNNER] Enabling debug mode...");
            TraderXDebugModeService.GetInstance().EnableDebugMode(true);
        }
        else
        {
            GetTraderXLogger().LogInfo("[TEST RUNNER] Debug mode disabled");
        }
        
        // Run test suites based on settings
        // if (debugSettings.runCurrencyServiceTests)
        // {
        //     RunCurrencyServiceTests();
        // }
        
        // if (debugSettings.runTransactionServiceTests)
        // {
        //     RunTransactionServiceTests();
        // }
        
        if (debugSettings.runPricingServiceTests)
        {
            //RunPricingServiceTests();
        }
        
        // if (debugSettings.runVehicleTransactionTests)
        // {
        //     RunVehicleTransactionTests();
        // }
        
        GetTraderXLogger().LogInfo("[TEST RUNNER] All configured test suites completed.");
    }


    TraderXDebugSettings GetDebugSettings()
    {
        if (!debugSettings)
            debugSettings = TraderXDebugSettingsRepository.Load();
        return debugSettings;
    }
    
    void RunCurrencyServiceTests()
    {
        GetTraderXLogger().LogInfo("[TEST RUNNER] Running Currency Service Tests...");
        
        // Create a new instance of the currency test class
        TraderXCurrencyServiceTest currencyTest = new TraderXCurrencyServiceTest();
        
        // Setup the test environment
        currencyTest.OnBeforeTest();
        
        // Run the tests
        currencyTest.StartUnitTest();
        
        // Cleanup (the test class handles its own cleanup)
        currencyTest = null;
        
        GetTraderXLogger().LogInfo("[TEST RUNNER] Currency Service Tests completed.");
    }

    void RunTransactionServiceTests()
    {
        GetTraderXLogger().LogInfo("[TEST RUNNER] Running Transaction Service Tests...");
        
        // Create a new instance of the transaction test class
        TraderXTransactionsTest transactionTest = new TraderXTransactionsTest();
        
        // Setup the test environment
        transactionTest.OnBeforeTest();
        
        // Run the tests
        transactionTest.StartUnitTest();
        
        // Cleanup (the test class handles its own cleanup)
        transactionTest = null;
        
        GetTraderXLogger().LogInfo("[TEST RUNNER] Transaction Service Tests completed.");
    }

    void RunPricingServiceTests()
    {
        GetTraderXLogger().LogInfo("[TEST RUNNER] Running Pricing Service Tests...");
        
        // Create a new instance of the pricing test class
        TraderXPricingServiceTest pricingTest = new TraderXPricingServiceTest();
        
        // Run the tests
        pricingTest.RunPricingServiceTests();
        
        // Cleanup (the test class handles its own cleanup)
        pricingTest = null;
        
        GetTraderXLogger().LogInfo("[TEST RUNNER] Pricing Service Tests completed.");
    }

    void RunVehicleTransactionTests()
    {
        GetTraderXLogger().LogInfo("[TEST RUNNER] Running Vehicle Transaction Tests...");
        
        // // Create a new instance of the vehicle transaction test class
        // TraderXVehicleTransactionTest vehicleTest = new TraderXVehicleTransactionTest();
        
        // // Run all vehicle tests
        // vehicleTest.RunAllTests();
        
        // // Cleanup (the test class handles its own cleanup)
        // vehicleTest = null;
        
        GetTraderXLogger().LogInfo("[TEST RUNNER] Vehicle Transaction Tests completed.");
    }

    override void OnMissionStart(Class sender, CF_EventArgs args)
	{
		super.OnMissionStart(sender, args);
        if(GetGame().IsServer())
        {
            GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(StartUnitTest, 3000);
        }
	}
}
