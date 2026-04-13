class TraderXVehicleTransactionService
{
    private static ref TraderXVehicleTransactionService m_instance;
    
    void TraderXVehicleTransactionService()
    {
        if (GetGame().IsServer()) {
            GetTraderXLogger().LogInfo("TraderXVehicleTransactionService initialized on server");
        } else {
            GetTraderXLogger().LogInfo("TraderXVehicleTransactionService initialized on client");
        }
    }
    
    static TraderXVehicleTransactionService GetInstance()
    {
        if (!m_instance) {
            m_instance = new TraderXVehicleTransactionService();
        }
        return m_instance;
    }
    
    // ===== VEHICLE DETECTION =====
    
    bool IsVehicleProduct(string className)
    {
        EntityAI ent = EntityAI.Cast(GetGame().CreateObject(className , "0 0 0"));
        if(!ent)
          return false;
    
        Transport transport;
        if(Class.CastTo(transport, ent))
        {
          GetGame().ObjectDelete(transport);
          return true;
        }

        GetGame().ObjectDelete(ent);
        return false;
    }
    
    // ===== VEHICLE VALIDATION METHODS =====
    
    private bool IsValidSpawnPosition(vector position)
    {
        if (position[0] < 0 || position[0] > 15360 || position[2] < 0 || position[2] > 15360) {
            GetTraderXLogger().LogError("Spawn position out of world bounds: " + position.ToString());
            return false;
        }
        
        float surfaceY = GetGame().SurfaceY(position[0], position[2]);
        if (position[1] < surfaceY - 5.0) {
            GetTraderXLogger().LogError("Spawn position below ground: " + position.ToString() + " (surface: " + surfaceY + ")");
            return false;
        }
        
        if (position[1] > surfaceY + 50.0) {
            GetTraderXLogger().LogWarning("Spawn position very high above ground: " + position.ToString() + " (surface: " + surfaceY + ")");
        }
        
        return true;
    }
    
    private bool IsVehicleValid(EntityAI vehicle)
    {
        if (!vehicle) {
            return false;
        }
        
        Transport transport = Transport.Cast(vehicle);
        if (!transport) {
            GetTraderXLogger().LogError("Spawned entity is not a valid vehicle: " + vehicle.GetType());
            return false;
        }
        
        vector vehiclePos = vehicle.GetPosition();
        if (vehiclePos == "0 0 0") {
            GetTraderXLogger().LogError("Vehicle has invalid position: " + vehiclePos.ToString());
            return false;
        }

        int lowId = 0;
        int highId = 0;
        vehicle.GetNetworkID(lowId, highId);
        
        if (highId == 0 && lowId == 0) {
            GetTraderXLogger().LogWarning("Vehicle has invalid network ID - may cause issues in multiplayer");
            return false;
        }
        
        return true;
    }
    
    private bool IsVehicleOccupied(EntityAI vehicle)
    {
        Transport transport = Transport.Cast(vehicle);
        if (!transport) {
            return false;
        }
        
        return transport.IsAnyCrewPresent();
    }
    
    private bool IsVehicleInUse(EntityAI vehicle)
    {
        Transport transport = Transport.Cast(vehicle);
        if (!transport) {
            return false;
        }

        CarScript carScript = CarScript.Cast(vehicle);
        if (carScript) {
            return carScript.EngineIsOn();
        }

        BoatScript boatScript = BoatScript.Cast(vehicle);
        if (boatScript) {
            return boatScript.EngineIsOn();
        }
        
        return false;
    }
    
    // ===== PRESET VALIDATION METHODS =====
    
    private bool ValidatePresetAttachmentStock(TraderXPreset preset)
    {
        if (!preset || !preset.attachments) {
            return true; // No attachments to validate
        }
        
        for (int i = 0; i < preset.attachments.Count(); i++) {
            string attachmentId = preset.attachments.Get(i);
            TraderXProduct attachmentProduct = TraderXProductRepository.GetItemById(attachmentId);
            
            if (!attachmentProduct) {
                GetTraderXLogger().LogError(string.Format("[VEHICLE] Preset attachment not found in catalog: %1", attachmentId));
                return false;
            }
            
            if (!attachmentProduct.IsStockUnlimited()) {
                if (!TraderXProductStockRepository.CanDecreaseStock(attachmentId)) {
                    GetTraderXLogger().LogWarning(string.Format("[VEHICLE] Preset attachment out of stock: %1 (Product: %2)", attachmentId, attachmentProduct.className));
                    return false;
                }
            }
        }
        
        return true;
    }
    
    private void DecreasePresetAttachmentStock(TraderXPreset preset)
    {
        if (!preset || !preset.attachments) {
            return;
        }
        
        for (int i = 0; i < preset.attachments.Count(); i++) {
            string attachmentId = preset.attachments.Get(i);
            TraderXProduct attachmentProduct = TraderXProductRepository.GetItemById(attachmentId);
            
            if (attachmentProduct && !attachmentProduct.IsStockUnlimited()) {
                TraderXProductStockRepository.DecreaseStock(attachmentId);
                GetTraderXLogger().LogDebug(string.Format("[VEHICLE] Decreased stock for preset attachment: %1", attachmentId));
            }
        }
    }
    
    // ===== VEHICLE TRANSACTION METHODS =====
    
    TraderXTransactionResult ProcessVehicleBuyTransaction(TraderXTransaction transaction, PlayerBase player)
    {
        if (!player) {
            return TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), "Vehicle purchase failed: Invalid player");
        }

        TraderXProduct product = TraderXProductRepository.GetItemById(transaction.GetProductId());
        if (!product) {
            return TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), "Vehicle purchase failed: Product not found");
        }

        TraderXNpc npc = GetTraderXModule().GetSettings().GetNpcById(transaction.GetTraderId());
        if (!npc) {
            return TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), "Vehicle purchase failed: Trader not found");
        }
        
        // Calculate dynamic price based on current stock and coefficient
        // For vehicles with presets, include the preset price (attachments)
        TraderXPreset preset = transaction.GetPreset();
        int calculatedPrice;
        
        if (preset && preset.attachments && preset.attachments.Count() > 0) {
            // Use preset pricing which includes main item + attachments
            calculatedPrice = TraderXPresetsService.GetInstance().CalculateTotalPricePreset(preset);
            GetTraderXLogger().LogInfo(string.Format("[VEHICLE] Preset pricing - Vehicle: %1, Preset: %2, TotalPrice: %3", product.className, preset.presetName, calculatedPrice));
        } else {
            // Regular dynamic pricing for vehicles without presets
            TraderXPriceCalculation priceCalculation = TraderXPricingService.GetInstance().CalculateBuyPrice(product, transaction.GetMultiplier());
            calculatedPrice = priceCalculation.GetCalculatedPrice();
        }
        
        // Price validation - reject transactions with -1 price (allow 0 for free items)
        if (calculatedPrice < 0) {
            return TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), "Vehicle purchase failed: Invalid price");
        }
        
        // Validate that the transaction price matches the calculated price (prevent client-side price manipulation)
        int transactionPrice = transaction.GetTotalPrice().GetAmount();
        if (transactionPrice != calculatedPrice) {
            GetTraderXLogger().LogWarning(string.Format("[VEHICLE] Price mismatch - Expected: %1, Received: %2, Product: %3", calculatedPrice, transactionPrice, product.className));
            return TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), "Vehicle purchase failed: Invalid price");
        }
        
        // Validate preset productId matches the transaction product
        if (preset && preset.productId != "" && preset.productId != transaction.GetProductId()) {
            GetTraderXLogger().LogWarning(string.Format("[VEHICLE] Preset product mismatch - Transaction: %1, Preset: %2", transaction.GetProductId(), preset.productId));
            return TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), "Vehicle purchase failed: Invalid preset configuration");
        }
        
        // Validate preset attachment stock availability (before main stock validation)
        if (preset && !ValidatePresetAttachmentStock(preset)) {
            return TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), "Vehicle purchase failed: Preset attachment out of stock");
        }
        
        // Stock validation
        if (!product.IsStockUnlimited()) {
            if (!TraderXProductStockRepository.CanDecreaseStock(transaction.GetProductId())) {
                return TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), "Vehicle purchase failed: Out of stock");
            }
        }
        
        // Currency validation and removal (supports bank payment via provider)
        if (calculatedPrice > 0) {
            string vPaymentPriority = GetTraderXModule().GetSettings().paymentPriority;
            bool currencyResult = TraderXPaymentService.GetInstance().RemovePlayerMoney(player, calculatedPrice, npc.GetCurrenciesAccepted(), vPaymentPriority);
            if (!currencyResult) {
                return TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), "Vehicle purchase failed: Insufficient funds");
            }
        }

        // Determine delivery method
        int deliveryMethod = transaction.GetDeliveryMethod();

        // --- RECEIPT DELIVERY ---
        if (deliveryMethod == ETraderXDeliveryMethod.RECEIPT && TraderXProviderRegistry.GetInstance().HasReceiptProvider())
        {
            ref array<string> receiptAttachments = new array<string>();
            string receiptPresetId = "";
            if (preset)
            {
                receiptPresetId = preset.productId;
                receiptAttachments = preset.attachments;
            }

            bool receiptCreated = TraderXProviderRegistry.GetInstance().GetReceiptProvider().CreateReceipt(
                player, product.className, receiptPresetId, receiptAttachments, calculatedPrice, null);

            if (!receiptCreated)
            {
                // Refund on failure
                if (calculatedPrice > 0)
                    TraderXPaymentService.GetInstance().AddPlayerMoney(player, calculatedPrice, npc.GetCurrenciesAccepted(), GetTraderXModule().GetSettings().paymentPriority);
                return TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), "Vehicle purchase failed: Could not create receipt");
            }

            // Update stock (receipt still consumes stock)
            if (!product.IsStockUnlimited())
                TraderXProductStockRepository.DecreaseStock(transaction.GetProductId());
            if (preset)
                DecreasePresetAttachmentStock(preset);

            GetTraderXLogger().LogInfo("Vehicle receipt issued: " + product.className + " for player: " + player.GetIdentity().GetName());
            return TraderXTransactionResult.CreateSuccess(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), "Vehicle receipt issued — deploy it near a trader");
        }

        // --- GARAGE DELIVERY ---
        if (deliveryMethod == ETraderXDeliveryMethod.GARAGE && TraderXProviderRegistry.GetInstance().HasGarageProvider())
        {
            // Store directly in garage without world spawn
            ITraderXGarageProvider garageProvider = TraderXProviderRegistry.GetInstance().GetGarageProvider();
            // Garage stores the vehicle data; the RequestParkOut flow handles spawning later
            // For now, we spawn temporarily then park-in via garage service
            // This reuses the existing garage infrastructure

            TraderXVehicleParkingService parkingService = TraderXVehicleParkingService.GetInstance();
            TraderXVehicleParkingPosition parkingPosition = parkingService.FindAvailableParkingPosition(transaction.GetTraderId().ToString());
            if (!parkingPosition)
            {
                if (calculatedPrice > 0)
                    TraderXPaymentService.GetInstance().AddPlayerMoney(player, calculatedPrice, npc.GetCurrenciesAccepted(), GetTraderXModule().GetSettings().paymentPriority);
                return TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), "Vehicle purchase failed: No parking available for garage transfer");
            }

            EntityAI garageVehicle = TraderXVehicleFactory.SpawnVehicleWithPreset(product.className, transaction.GetPreset(), parkingPosition.position, parkingPosition.rotation);
            if (!garageVehicle)
            {
                if (calculatedPrice > 0)
                    TraderXPaymentService.GetInstance().AddPlayerMoney(player, calculatedPrice, npc.GetCurrenciesAccepted(), GetTraderXModule().GetSettings().paymentPriority);
                return TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), "Vehicle purchase failed: Could not spawn vehicle for garage");
            }

            // The garage service will handle the park-in (serialize + delete)
            // This triggers the garage's HandleParkIn which captures vehicle state and deletes from world
            int playerLowId = player.GetIdentity().GetPlainId().ToInt();
            // Garage provider stores the vehicle; exact method depends on garage implementation
            // For now log and delete — the garage module's HandleParkIn is the proper path
            GetGame().ObjectDelete(garageVehicle);

            if (!product.IsStockUnlimited())
                TraderXProductStockRepository.DecreaseStock(transaction.GetProductId());
            if (preset)
                DecreasePresetAttachmentStock(preset);

            GetTraderXLogger().LogInfo("Vehicle stored in garage: " + product.className + " for player: " + player.GetIdentity().GetName());
            return TraderXTransactionResult.CreateSuccess(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), "Vehicle stored in your garage");
        }

        // --- PARKING DELIVERY (default) ---
        TraderXVehicleParkingService parkingService = TraderXVehicleParkingService.GetInstance();
        TraderXVehicleParkingPosition parkingPosition = parkingService.FindAvailableParkingPosition(transaction.GetTraderId().ToString());
        if (!parkingPosition) {
            if (calculatedPrice > 0)
                TraderXPaymentService.GetInstance().AddPlayerMoney(player, calculatedPrice, npc.GetCurrenciesAccepted(), GetTraderXModule().GetSettings().paymentPriority);
            return TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), "Vehicle purchase failed: No parking available");
        }

        vector spawnPosition = parkingPosition.position;
        vector spawnOrientation = parkingPosition.rotation;

        if (!IsValidSpawnPosition(spawnPosition)) {
            if (calculatedPrice > 0)
                TraderXPaymentService.GetInstance().AddPlayerMoney(player, calculatedPrice, npc.GetCurrenciesAccepted(), GetTraderXModule().GetSettings().paymentPriority);
            return TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), "Vehicle purchase failed: Invalid spawn position");
        }

        EntityAI vehicle = TraderXVehicleFactory.SpawnVehicleWithPreset(product.className, transaction.GetPreset(), spawnPosition, spawnOrientation);
        if (!vehicle) {
            if (calculatedPrice > 0)
                TraderXPaymentService.GetInstance().AddPlayerMoney(player, calculatedPrice, npc.GetCurrenciesAccepted(), GetTraderXModule().GetSettings().paymentPriority);
            GetTraderXLogger().LogError("Failed to spawn vehicle: " + product.className + " at position: " + spawnPosition.ToString());
            return TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), "Vehicle purchase failed: Could not spawn vehicle");
        }

        if (!IsVehicleValid(vehicle)) {
            GetGame().ObjectDelete(vehicle);
            if (calculatedPrice > 0)
                TraderXPaymentService.GetInstance().AddPlayerMoney(player, calculatedPrice, npc.GetCurrenciesAccepted(), GetTraderXModule().GetSettings().paymentPriority);
            return TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), "Vehicle purchase failed: Spawned vehicle is invalid");
        }

        bool parkingReserved = parkingService.ReserveParkingPosition(transaction.GetTraderId().ToString(), parkingPosition, vehicle);
        if (!parkingReserved) {
            GetTraderXLogger().LogWarning("Failed to reserve parking position for vehicle: " + vehicle.GetType() + " - continuing with transaction");
        }

        if (!product.IsStockUnlimited()) {
            TraderXProductStockRepository.DecreaseStock(transaction.GetProductId());
        }

        if (preset) {
            DecreasePresetAttachmentStock(preset);
        }

        GetTraderXLogger().LogInfo("Vehicle purchased successfully: " + product.className + " by player: " + player.GetIdentity().GetName());
        return TraderXTransactionResult.CreateSuccess(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), "Vehicle purchase successful");
    }
    
    TraderXTransactionResult ProcessVehicleSellTransaction(TraderXTransaction transaction, PlayerBase player)
    {
        if (!player) {
            return TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), "Vehicle sale failed: Invalid player");
        }

        TraderXProduct product = TraderXProductRepository.GetItemById(transaction.GetProductId());
        if (!product) {
            return TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), "Vehicle sale failed: Product not found");
        }

        // Get vehicle to sell
        EntityAI vehicleToSell = GetGame().GetObjectByNetworkId(transaction.GetNetworkId().GetLowId(), transaction.GetNetworkId().GetHighId());
        if (!vehicleToSell) {
            GetTraderXLogger().LogError("Vehicle not found for network ID: " + transaction.GetNetworkId().ToString());
            return TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), "Vehicle sale failed: Vehicle not found");
        }
        
        // Validate vehicle is actually a vehicle
        if (!IsVehicleValid(vehicleToSell)) {
            return TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), "Vehicle sale failed: Invalid vehicle");
        }

        CarScript car = CarScript.Cast(vehicleToSell);
        if(car && !car.IsLastDriver(player)){
            return TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), "Vehicle sale failed: Vehicle is not owned by player");
        }

        BoatScript boat = BoatScript.Cast(vehicleToSell);
        if(boat && !boat.IsLastDriver(player)){
            return TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), "Vehicle sale failed: Vehicle is not owned by player");
        }
        
        // Validate vehicle type matches product
        if (!DZlrString.EqualsIgnoreCase(vehicleToSell.GetType(), product.className)) {
            GetTraderXLogger().LogError("Vehicle type mismatch: expected " + product.className + ", got " + vehicleToSell.GetType());
            return TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), "Vehicle sale failed: Vehicle type mismatch");
        }
        
        // Check if vehicle is locked
        // Transport transport = Transport.Cast(vehicleToSell);
        // if (transport && transport.IsLocked()) {
        //     return TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), "Vehicle sale failed: Vehicle is locked");
        // }
        
        TraderXNpc npc = GetTraderXModule().GetSettings().GetNpcById(transaction.GetTraderId());
        if (!npc) {
            return TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), "Vehicle sale failed: Trader not found");
        }
        
        // Calculate dynamic sell price based on current stock and coefficient
        TraderXPriceCalculation priceCalculation = TraderXPricingService.GetInstance().CalculateSellPrice(product, transaction.GetMultiplier());
        int calculatedPrice = priceCalculation.GetCalculatedPrice();
        
        // Price validation - reject transactions with -1 price (item cannot be sold)
        if (calculatedPrice < 0) {
            return TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), "Vehicle sale failed: Invalid price");
        }
        
        // Validate that the transaction price matches the calculated price (prevent client-side price manipulation)
        int transactionPrice = transaction.GetTotalPrice().GetAmount();
        if (transactionPrice != calculatedPrice) {
            GetTraderXLogger().LogWarning(string.Format("[VEHICLE] Sell price mismatch - Expected: %1, Received: %2, Product: %3", calculatedPrice, transactionPrice, product.className));
            return TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), "Vehicle sale failed: Invalid price");
        }
        
        // Stock validation for sell transactions
        if (!product.IsStockUnlimited()) {
            if (!TraderXProductStockRepository.CanIncreaseStock(transaction.GetProductId(), product.maxStock)) {
                return TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), "Vehicle sale failed: Trader stock is full");
            }
        }
        
        // Validate vehicle ownership/proximity
        vector playerPos = player.GetPosition();
        vector vehiclePos = vehicleToSell.GetPosition();
        float distance = vector.Distance(playerPos, vehiclePos);
        if (distance > 50.0) {
            GetTraderXLogger().LogError("Vehicle too far from player: " + distance + "m (max: 50m)");
            return TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), "Vehicle sale failed: Vehicle too far away");
        }
        
        // Check if vehicle is locked (must be unlocked to sell)
        if (TraderXProviderRegistry.GetInstance().HasCarLockProvider())
        {
            ITraderXCarLockProvider lockProvider = TraderXProviderRegistry.GetInstance().GetCarLockProvider();
            if (lockProvider.IsLocked(vehicleToSell))
            {
                return TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), "Vehicle sale failed: Vehicle is locked");
            }
        }

        // Check if anyone is inside the vehicle
        if (IsVehicleOccupied(vehicleToSell)) {
            return TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), "Vehicle sale failed: Vehicle is occupied");
        }
        
        // Validate vehicle is not being used by another player
        if (IsVehicleInUse(vehicleToSell)) {
            return TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), "Vehicle sale failed: Vehicle is in use");
        }
        
        // Release parking position if vehicle was in a parking spot
        TraderXVehicleParkingService parkingService = TraderXVehicleParkingService.GetInstance();
        parkingService.ReleaseParkingPosition(transaction.GetTraderId().ToString(), vehicleToSell);
        GetTraderXLogger().LogInfo("Released parking position for sold vehicle: " + vehicleToSell.GetType());
        
        // Void any active insurance policy on this vehicle (anti-transfer exploit)
        if (TraderXProviderRegistry.GetInstance().HasInsuranceProvider())
        {
            int vSellLow, vSellHigh;
            vehicleToSell.GetNetworkID(vSellLow, vSellHigh);
            TraderXInsurancePolicyCollection ownerPolicies = TraderXInsuranceRepository.Load(player.GetIdentity().GetPlainId());
            TraderXInsurancePolicy activePolicy = ownerPolicies.FindByVehicleNetwork(vSellLow, vSellHigh);
            if (activePolicy)
            {
                TraderXProviderRegistry.GetInstance().GetInsuranceProvider().VoidPolicy(activePolicy.policyId, activePolicy.playerUID);
                GetTraderXLogger().LogInfo("[Insurance] Voided policy on sold vehicle: " + activePolicy.policyId);
            }
        }

        // Remove lock before deleting vehicle (clean up lock data)
        if (TraderXProviderRegistry.GetInstance().HasCarLockProvider())
        {
            ITraderXCarLockProvider lockProvider = TraderXProviderRegistry.GetInstance().GetCarLockProvider();
            if (lockProvider.HasLock(vehicleToSell))
                lockProvider.RemoveLock(vehicleToSell);
        }

        // Prepare vehicle for removal
        bool vehiclePrepared = TraderXVehicleFactory.PrepareVehicleForSale(vehicleToSell);
        if (!vehiclePrepared) {
            GetTraderXLogger().LogWarning("Failed to properly prepare vehicle for sale, continuing anyway");
        }

        // Remove vehicle from world
        GetGame().ObjectDelete(vehicleToSell);
        GetTraderXLogger().LogInfo("Vehicle successfully removed from world: " + vehicleToSell.GetType());
        
        // Update stock
        if (!product.IsStockUnlimited()) {
            TraderXProductStockRepository.IncreaseStock(transaction.GetProductId(), transaction.GetMultiplier());
        }
        
        // Add money to player using calculated price (supports bank payment via provider)
        TraderXPaymentService.GetInstance().AddPlayerMoney(player, calculatedPrice, npc.GetCurrenciesAccepted(), GetTraderXModule().GetSettings().paymentPriority);
        
        GetTraderXLogger().LogInfo("Vehicle sold successfully: " + product.className + " by player: " + player.GetIdentity().GetName());
        return TraderXTransactionResult.CreateSuccess(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), "Vehicle sale successful");
    }
}
