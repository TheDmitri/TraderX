// class TraderXVehicleTransactionTest
// {
//     private PlayerBase m_testPlayer;
//     private TraderXNpc m_testNpc;
//     private TraderXProduct m_testVehicleProduct;
//     private int m_testsPassed = 0;
//     private int m_testsFailed = 0;
    
//     void TraderXVehicleTransactionTest()
//     {
//         GetTraderXLogger().LogDebug("Starting Vehicle Transaction Tests VehicleTransactionTest");
//     }
    
//     void RunAllTests()
//     {
//         OnBeforeTest();
        
//         // Test parking system
//         TestParkingConfigLoading();
//         TestParkingCollectionLoading();
//         TestParkingAvailability();
        
//         // Test vehicle factory
//         TestVehiclePresetLoading();
//         TestVehicleSpawning();
        
//         // Test vehicle transactions
//         TestVehicleBuyTransaction();
//         TestVehicleSellTransaction();
//         TestVehicleBuyWithNoParkingAvailable();
//         TestVehicleSellWithInvalidVehicle();
        
//         PrintTestSummary();
//     }
    
//     private void OnBeforeTest()
//     {
//         GetTraderXLogger().LogDebug("Setting up test environment VehicleTransactionTest");
        
//         // Create test player
//         m_testPlayer = PlayerBase.Cast(GetGame().CreatePlayer(null, GetGame().CreateRandomPlayer(), "7500 0 7500", 0, "NONE"));
//         if (!m_testPlayer) {
//             GetTraderXLogger().LogError("Failed to create test player VehicleTransactionTest");
//             return;
//         }
        
//         // Create test NPC
//         vector npcPos = "7495 0 7495";
//         m_testNpc = new TraderXNpc("TRADER_001", "Test Vehicle Trader", npcPos, "0 0 0", "SurvivorM_Mirek", true, true, true, true);
        
//         // Create test vehicle product
//         m_testVehicleProduct = new TraderXProduct();
//         m_testVehicleProduct.id = "VEHICLE_001";
//         m_testVehicleProduct.displayName = "Test Offroad Hatchback";
//         m_testVehicleProduct.className = "OffroadHatchback";
//         m_testVehicleProduct.buyPrice = 50000;
//         m_testVehicleProduct.sellPrice = 25000;
//         m_testVehicleProduct.stock = 5;
//         m_testVehicleProduct.maxStock = 10;
        
//         // Add currency to player for testing
//         TraderXCurrencyService.GetInstance().AddMoneyToPlayer(m_testPlayer, 100000, m_testNpc.GetCurrenciesAccepted());
        
//         GetTraderXLogger().LogDebug("Test environment setup complete", "VehicleTransactionTest");
//     }
    
//     private void TestParkingConfigLoading()
//     {
//         GetTraderXLogger().LogDebug("Testing parking config loading", "VehicleTransactionTest");
        
//         TraderXVehicleParkingConfig config = TraderXVehicleParkingRepository.LoadParkingConfig();
        
//         if (config && config.whitelistedObjects && config.whitelistedObjects.Count() > 0) {
//             AssertTrue(true, "Parking config loaded successfully with " + config.whitelistedObjects.Count() + " whitelisted objects");
//         } else {
//             AssertTrue(false, "Failed to load parking config or config is empty");
//         }
//     }
    
//     private void TestParkingCollectionLoading()
//     {
//         GetTraderXLogger().LogDebug("Testing parking collection loading", "VehicleTransactionTest");
        
//         TraderXVehicleParkingCollection collection = TraderXVehicleParkingRepository.LoadParkingCollection("TRADER_001");
        
//         if (collection && collection.parkingPositions && collection.parkingPositions.Count() > 0) {
//             AssertTrue(true, "Parking collection loaded successfully with " + collection.parkingPositions.Count() + " parking positions");
//         } else {
//             AssertTrue(false, "Failed to load parking collection or collection is empty");
//         }
//     }
    
//     private void TestParkingAvailability()
//     {
//         GetTraderXLogger().LogDebug("Testing parking availability", "VehicleTransactionTest");
        
//         TraderXVehicleParkingService parkingService = TraderXVehicleParkingService.GetInstance();
//         TraderXVehicleParkingPosition availablePosition = parkingService.FindAvailableParkingPosition("TRADER_001");
        
//         if (availablePosition) {
//             AssertTrue(true, "Found available parking position at " + availablePosition.position.ToString());
//         } else {
//             AssertTrue(false, "No available parking positions found");
//         }
//     }
    
//     private void TestVehiclePresetLoading()
//     {
//         GetTraderXLogger().LogDebug("Testing vehicle preset loading VehicleTransactionTest");
        
//         TraderXPreset preset = TraderXPresetRepository.GetVehiclePreset("OffroadHatchback", "OffroadHatchback_Basic");
        
//         if (preset) {
//             AssertTrue(true, "Vehicle preset loaded successfully: " + preset.presetName);
//         } else {
//             AssertTrue(false, "Failed to load vehicle preset");
//         }
//     }
    
//     private void TestVehicleSpawning()
//     {
//         GetTraderXLogger().LogDebug("Testing vehicle spawning VehicleTransactionTest");
        
//         TraderXPreset preset = TraderXPresetRepository.GetVehiclePreset("OffroadHatchback", "OffroadHatchback_Basic");
//         vector spawnPos = "7600 0 7600";
//         vector spawnOrientation = "0 0 0";
        
//         EntityAI vehicle = TraderXVehicleFactory.SpawnVehicleWithPreset("OffroadHatchback", preset, spawnPos, spawnOrientation);
        
//         if (vehicle) {
//             AssertTrue(true, "Vehicle spawned successfully at " + spawnPos.ToString());
//             // Clean up test vehicle
//             GetGame().ObjectDelete(vehicle);
//         } else {
//             AssertTrue(false, "Failed to spawn vehicle");
//         }
//     }
    
//     private void TestVehicleBuyTransaction()
//     {
//         GetTraderXLogger().LogDebug("Testing vehicle buy transaction VehicleTransactionTest");
        
//         TraderXTransaction transaction = TraderXTransaction.CreateBuyTransaction("TX_001", "VEHICLE_001", "TRADER_001", 1);
//         TraderXVehicleTransactionService vehicleTransactionService = TraderXVehicleTransactionService.GetInstance();
        
//         TraderXTransactionResult result = vehicleTransactionService.ProcessVehicleBuyTransaction(transaction, m_testPlayer);
        
//         if (result && result.IsSuccess()) {
//             AssertTrue(true, "Vehicle buy transaction completed successfully");
//         } else {
//             string errorMsg;
//             if (result) {
//                 errorMsg = result.GetErrorMessage();
//             } else {
//                 errorMsg = "Unknown error";
//             }
//             AssertTrue(false, "Vehicle buy transaction failed: " + errorMsg);
//         }
//     }
    
//     private void TestVehicleSellTransaction()
//     {
//         GetTraderXLogger().LogDebug("Testing vehicle sell transaction VehicleTransactionTest");
        
//         // First spawn a vehicle near the player for selling
//         vector vehiclePos = m_testPlayer.GetPosition() + "2 0 0";
//         EntityAI testVehicle = GetGame().CreateObject("OffroadHatchback", vehiclePos);
        
//         if (!testVehicle) {
//             AssertTrue(false, "Failed to create test vehicle for sell transaction");
//             return;
//         }
        
//         TraderXTransaction transaction = TraderXTransaction.CreateSellTransaction("TX_002", "VEHICLE_001", "TRADER_001", 1);
//         transaction.SetNetworkId(testVehicle.GetNetworkId());
        
//         TraderXVehicleTransactionService vehicleTransactionService = TraderXVehicleTransactionService.GetInstance();
//         TraderXTransactionResult result = vehicleTransactionService.ProcessVehicleSellTransaction(transaction, m_testPlayer);
        
//         if (result && result.IsSuccess()) {
//             AssertTrue(true, "Vehicle sell transaction completed successfully");
//         } else {
//             string errorMsg;
//             if (result) {
//                 errorMsg = result.GetErrorMessage();
//             } else {
//                 errorMsg = "Unknown error";
//             }
//             AssertTrue(false, "Vehicle sell transaction failed: " + errorMsg);
//             // Clean up test vehicle if transaction failed
//             GetGame().ObjectDelete(testVehicle);
//         }
//     }
    
//     private void TestVehicleBuyWithNoParkingAvailable()
//     {
//         GetTraderXLogger().LogDebug("Testing vehicle buy with no parking available VehicleTransactionTest");
        
//         TraderXVehicleParkingService parkingService = TraderXVehicleParkingService.GetInstance();
        
//         // Reserve all parking positions to simulate no availability
//         TraderXVehicleParkingCollection collection = TraderXVehicleParkingRepository.LoadParkingCollection("TRADER_001");
//         if (collection && collection.parkingPositions) {
//             for (int i = 0; i < collection.parkingPositions.Count(); i++) {
//                 TraderXVehicleParkingPosition pos = collection.parkingPositions.Get(i);
//                 parkingService.ReserveParkingPosition("TRADER_001", pos);
//             }
//         }
        
//         TraderXTransaction transaction = TraderXTransaction.CreateBuyTransaction("TX_003", "VEHICLE_001", "TRADER_001", 1);
//         TraderXVehicleTransactionService vehicleTransactionService = TraderXVehicleTransactionService.GetInstance();
        
//         TraderXTransactionResult result = vehicleTransactionService.ProcessVehicleBuyTransaction(transaction, m_testPlayer);
        
//         if (result && !result.IsSuccess() && result.GetErrorMessage().Contains("parking")) {
//             AssertTrue(true, "Vehicle buy correctly failed when no parking available");
//         } else {
//             AssertTrue(false, "Vehicle buy should have failed due to no parking availability");
//         }
        
//         // Clean up - release all parking positions
//         if (collection && collection.parkingPositions) {
//             for (int i = 0; i < collection.parkingPositions.Count(); i++) {
//                 TraderXVehicleParkingPosition pos = collection.parkingPositions.Get(i);
//                 parkingService.ReleaseParkingPosition("TRADER_001", pos);
//             }
//         }
//     }
    
//     private void TestVehicleSellWithInvalidVehicle()
//     {
//         GetTraderXLogger().LogDebug("Testing vehicle sell with invalid vehicle VehicleTransactionTest");
        
//         TraderXTransaction transaction = TraderXTransaction.CreateSellTransaction("TX_004", "VEHICLE_001", "TRADER_001", 1);
//         transaction.SetNetworkId(999999); // Invalid network ID
        
//         TraderXVehicleTransactionService vehicleTransactionService = TraderXVehicleTransactionService.GetInstance();
//         TraderXTransactionResult result = vehicleTransactionService.ProcessVehicleSellTransaction(transaction, m_testPlayer);
        
//         if (result && !result.IsSuccess() && result.GetErrorMessage().Contains("vehicle")) {
//             AssertTrue(true, "Vehicle sell correctly failed with invalid vehicle");
//         } else {
//             AssertTrue(false, "Vehicle sell should have failed with invalid vehicle");
//         }
//     }
    
//     private void AssertTrue(bool condition, string message)
//     {
//         if (condition) {
//             m_testsPassed++;
//             GetTraderXLogger().LogDebug("[PASS] " + message, "VehicleTransactionTest");
//         } else {
//             m_testsFailed++;
//             TraderXLogger.Error("[FAIL] " + message, "VehicleTransactionTest");
//         }
//     }
    
//     private void PrintTestSummary()
//     {
//         GetTraderXLogger().LogDebug("=== Vehicle Transaction Test Summary ===", "VehicleTransactionTest");
//         GetTraderXLogger().LogDebug("Tests Passed: " + m_testsPassed + "VehicleTransactionTest");
//         GetTraderXLogger().LogDebug("Tests Failed: " + m_testsFailed + "VehicleTransactionTest");
//         GetTraderXLogger().LogDebug("Total Tests: " + (m_testsPassed + m_testsFailed) + "VehicleTransactionTest");
        
//         if (m_testsFailed == 0) {
//             GetTraderXLogger().LogDebug("All vehicle transaction tests PASSED! VehicleTransactionTest");
//         } else {
//             TraderXLogger.Error(m_testsFailed + " vehicle transaction tests FAILED! VehicleTransactionTest");
//         }
        
//         // Clean up test environment
//         if (m_testPlayer) {
//             GetGame().ObjectDelete(m_testPlayer);
//         }
//     }
// }
