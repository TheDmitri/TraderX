class TraderXTransactionService
{
    static ref TraderXTransactionService m_instance;

    float deltaTime = 0.0;
    
    const float TRADERX_QUEUE_TRANSACTION_INTERVAL = 0.5;
    ref TransactionQueue<ref TraderXTransactionRequest> m_TransactionRequestQueue;
    ref TransactionQueue<ref TraderXTransactionResultCollection> m_TransactionResponseQueue;
    
    
    void TraderXTransactionService()
    {
        m_TransactionRequestQueue = new TransactionQueue<ref TraderXTransactionRequest>();
        m_TransactionResponseQueue = new TransactionQueue<ref TraderXTransactionResultCollection>();
    }
    
    static TraderXTransactionService GetInstance()
    {
        if (!m_instance)
            m_instance = new TraderXTransactionService();
        return m_instance;
    }

    void RegisterRPCs()
    {
        if(GetGame().IsServer())
        {
            GetRPCManager().AddRPC("TraderX", "GetTransactionsRequest", this, SingeplayerExecutionType.Server);
        }
        else
        {
            GetRPCManager().AddRPC("TraderX", "OnTransactionsResponse", this, SingeplayerExecutionType.Client);
            GetRPCManager().AddRPC("TraderX", "OnStockUpdateResponse", this, SingeplayerExecutionType.Client);
        }
    }
    
    TraderXTransactionResult ProcessTransaction(TraderXTransaction transaction, PlayerBase player)
    {
        if (!transaction || !transaction.IsValid()) {
            TraderXDynamicTranslationSettings settings = TraderXDynamicTranslationRepository.GetSettings();
            return TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), settings.invalidTransaction);
        }
        
        // Check if this is a vehicle transaction
        TraderXProduct product = TraderXProductRepository.GetItemById(transaction.GetProductId());
        if (product && TraderXVehicleTransactionService.GetInstance().IsVehicleProduct(product.className)) {
            if (transaction.IsBuy())
                return TraderXVehicleTransactionService.GetInstance().ProcessVehicleBuyTransaction(transaction, player);
            else
                return TraderXVehicleTransactionService.GetInstance().ProcessVehicleSellTransaction(transaction, player);
        }
        
        // Regular item transaction
        if (transaction.IsBuy())
            return ProcessBuyTransaction(transaction, player);
        else
            return ProcessSellTransaction(transaction, player);
    }
    
    private TraderXTransactionResult ProcessBuyTransaction(TraderXTransaction transaction, PlayerBase player)
    {
        int i = 0;
        TraderXDynamicTranslationSettings settings = TraderXDynamicTranslationRepository.GetSettings();
        
        // Vérification des prérequis
        if (!player) {
            return TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), settings.purchaseFailedPrefix + settings.invalidPlayer);
        }

        // Récupération des données nécessaires
        TraderXProduct product = TraderXProductRepository.GetItemById(transaction.GetProductId());
        if (!product) {
            return TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), settings.purchaseFailedPrefix + settings.productNotFound);
        }

        TraderXNpc npc = GetTraderXModule().GetSettings().GetNpcById(transaction.GetTraderId());
        GetTraderXLogger().LogInfo(string.Format("[TRANSACTION] Retrieved NPC: %1, TraderId: %2", npc != null, transaction.GetTraderId()));
        if (npc) {
            GetTraderXLogger().LogInfo(string.Format("[TRANSACTION] NPC currencies count: %1", npc.GetCurrenciesAccepted().Count()));
        }
        
        // Calculate price using preset service if preset exists, otherwise use regular pricing
        TraderXPreset preset = transaction.GetPreset();
        int calculatedPrice;
        
        if (preset && preset.attachments && preset.attachments.Count() > 0) {
            // Use progressive preset pricing for multipliers to match client calculation
            if (transaction.GetMultiplier() > 1) {
                calculatedPrice = TraderXPresetsService.GetInstance().CalculateProgressivePresetPrice(preset, transaction.GetMultiplier());
                GetTraderXLogger().LogInfo(string.Format("[TRANSACTION] Progressive preset pricing - Multiplier: %1, Total: %2", transaction.GetMultiplier(), calculatedPrice));
            } else {
                // Single preset - use simple calculation
                calculatedPrice = TraderXPresetsService.GetInstance().CalculateTotalPricePreset(preset);
                GetTraderXLogger().LogInfo(string.Format("[TRANSACTION] Single preset pricing - Total: %1", calculatedPrice));
            }
        } else {
            // Regular dynamic pricing for non-preset items
            TraderXPriceCalculation priceCalculation = TraderXPricingService.GetInstance().CalculateBuyPrice(product, transaction.GetMultiplier());
            calculatedPrice = priceCalculation.GetCalculatedPrice();
        }
        
        // Enhanced debugging for price calculation
        int currentStock = TraderXProductStockRepository.GetStockAmount(product.GetProductId());
        GetTraderXLogger().LogInfo(string.Format("[TRANSACTION] Price calculation details - Product: %1 (ID: %2), BasePrice: %3, Coefficient: %4, CurrentStock: %5, Multiplier: %6, CalculatedPrice: %7", 
            product.className, product.GetProductId(), product.buyPrice, product.coefficient, currentStock, transaction.GetMultiplier(), calculatedPrice));
        
        // Price validation - reject transactions with -1 price (allow 0 for free items)
        if (calculatedPrice < 0) {
            GetTraderXLogger().LogError(string.Format("[TRANSACTION] Invalid calculated price: %1 for product: %2", calculatedPrice, product.className));
            return TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), settings.purchaseFailedPrefix + settings.invalidPrice);
        }
        
        // Validate that the transaction price matches the calculated price (prevent client-side price manipulation)
        int transactionPrice = transaction.GetTotalPrice().GetAmount();
        
        // For preset transactions with multipliers, allow small rounding differences due to integer division
        bool priceValid = false;
        if (preset && preset.attachments && preset.attachments.Count() > 0 && transaction.GetMultiplier() > 1) {
            // Allow up to 1€ per unit difference for preset multipliers due to rounding
            int maxAllowedDifference = transaction.GetMultiplier();
            int priceDifference = Math.AbsInt(calculatedPrice - transactionPrice);
            priceValid = (priceDifference <= maxAllowedDifference);
            
            GetTraderXLogger().LogInfo(string.Format("[TRANSACTION] Preset price validation - Expected: %1, Received: %2, Difference: %3, MaxAllowed: %4, Valid: %5", 
                calculatedPrice, transactionPrice, priceDifference, maxAllowedDifference, priceValid));
        } else {
            // Exact match required for non-preset transactions
            priceValid = (transactionPrice == calculatedPrice);
        }
        
        if (!priceValid) {
            GetTraderXLogger().LogWarning(string.Format("[TRANSACTION] Price mismatch - Expected: %1, Received: %2, Product: %3 (ID: %4), Stock: %5, Coefficient: %6", 
                calculatedPrice, transactionPrice, product.className, product.GetProductId(), currentStock, product.coefficient));
            return TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), settings.purchaseFailedPrefix + settings.invalidPrice);
        }
        
        // Vehicle multiplier validation - vehicles can only be purchased one at a time
        if (TraderXVehicleTransactionService.GetInstance().IsVehicleProduct(product.className)) {
            if (transaction.GetMultiplier() > 1) {
                return TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), settings.purchaseFailedPrefix + settings.vehicleMultipleQuantity);
            }
        }
        
        // Stock validation for main product
        if (!product.IsStockUnlimited()) {
            TraderXProductStock productStock = TraderXProductStockRepository.GetStockByProductId(transaction.GetProductId());
            if (!productStock || productStock.GetStock() < transaction.GetMultiplier()) {
                return TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), settings.purchaseFailedPrefix + settings.insufficientStock);
            }
        }
        
        // Preset attachment stock validation (if preset exists)
        if (preset && preset.attachments && preset.attachments.Count() > 0) {
            // SECURITY: Validate preset integrity first
            if (!TraderXPresetValidationService.GetInstance().ValidatePresetIntegrity(preset)) {
                return TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), settings.invalidPresetDetected);
            }
            
            // SECURITY: Validate all preset items are available from this specific trader
            TraderXTransactionResult traderValidation = TraderXPresetValidationService.GetInstance().ValidatePresetForTrader(player, preset, transaction.GetTraderId().ToString(), transaction);
            if (!traderValidation.IsSuccess()) {
                return traderValidation;
            }
            
            // Stock validation for attachments
            TraderXTransactionResult presetValidation = ValidatePresetAttachmentStock(preset, transaction);
            if (!presetValidation.IsSuccess()) {
                return presetValidation;
            }
        }
        
        // Création de l'item principal
        int baseQuantity = TraderXTradeQuantity.GetItemBuyQuantity(product.className, product.tradeQuantity);
        array<ItemBase> allCreatedItems = new array<ItemBase>();
        int j = 0;
        TraderXTransactionResult presetResult;
        // For weapons with presets and multiplier > 1, create each weapon individually to apply presets
        if (preset && preset.attachments && preset.attachments.Count() > 0 && transaction.GetMultiplier() > 1) {
            // Create each weapon individually and apply preset to each
            for (i = 0; i < transaction.GetMultiplier(); i++) {
                ItemBase weaponItem = TraderXItemFactory.CreateInInventory(player, product.className, baseQuantity);
                if (!weaponItem) {
                    // Rollback: remove all previously created items
                    for (j = 0; j < allCreatedItems.Count(); j++) {
                        GetGame().ObjectDelete(allCreatedItems[j]);
                    }
                    return TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), settings.purchaseFailedPrefix + string.Format(settings.couldNotCreateWeapon, (i + 1).ToString()));
                }
                
                allCreatedItems.Insert(weaponItem);
                
                // Apply preset to this weapon
                presetResult = ApplyPresetToItem(weaponItem, preset, transaction);
                if (!presetResult.IsSuccess()) {
                    // Rollback: remove all created items including this one
                    for (j = 0; j < allCreatedItems.Count(); j++) {
                        GetGame().ObjectDelete(allCreatedItems[j]);
                    }
                    return presetResult;
                }
            }
        } else {
            // Standard creation for items without presets - create each item individually for multiplier > 1
            for (i = 0; i < transaction.GetMultiplier(); i++) {
                ItemBase item = TraderXItemFactory.CreateInInventory(player, product.className, baseQuantity);
                if (!item) {
                    // Rollback: remove all previously created items
                    for (j = 0; j < allCreatedItems.Count(); j++) {
                        GetGame().ObjectDelete(allCreatedItems[j]);
                    }
                    return TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), settings.purchaseFailedPrefix + string.Format(settings.couldNotCreateItem + " (item %1 of %2)", (i + 1).ToString(), transaction.GetMultiplier().ToString()));
                }
                
                allCreatedItems.Insert(item);
                GetTraderXLogger().LogDebug(string.Format("[TRANSACTION] Created item %1 of %2: %3", (i + 1), transaction.GetMultiplier(), product.className));
                
                // Application du preset (attachments) for each item if preset exists
                if (preset && preset.attachments && preset.attachments.Count() > 0) {
                    presetResult = ApplyPresetToItem(item, preset, transaction);
                    if (!presetResult.IsSuccess()) {
                        // Rollback: remove all created items including this one
                        for (j = 0; j < allCreatedItems.Count(); j++) {
                            GetGame().ObjectDelete(allCreatedItems[j]);
                        }
                        return presetResult;
                    }
                }
            }
        }
        
        // Validate that we created the expected number of items
        if (allCreatedItems.Count() != transaction.GetMultiplier()) {
            GetTraderXLogger().LogError(string.Format("[TRANSACTION] Item creation validation failed: expected %1, created %2", transaction.GetMultiplier(), allCreatedItems.Count()));
            // Rollback: remove all created items
            for (j = 0; j < allCreatedItems.Count(); j++) {
                GetGame().ObjectDelete(allCreatedItems[j]);
            }
            return TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), settings.purchaseFailedPrefix + "Item creation count validation failed");
        }
        
        GetTraderXLogger().LogInfo(string.Format("[TRANSACTION] Successfully created %1 items of type %2", allCreatedItems.Count(), product.className));
        
        // Mise à jour du stock principal
        if (!product.IsStockUnlimited()) {
            TraderXProductStockRepository.DecreaseStock(transaction.GetProductId(), transaction.GetMultiplier());
        }
        
        // Mise à jour du stock des attachments
        if (preset && preset.attachments && preset.attachments.Count() > 0) {
            UpdatePresetAttachmentStock(preset);
        }
        
        // Retrait de la monnaie - skip if price is zero (free items)
        int tTotalPrice = transaction.GetTotalPrice().GetAmount();
        GetTraderXLogger().LogInfo(string.Format("[TRANSACTION] Processing payment: totalPrice=%1, npcCurrencies=%2", tTotalPrice, npc.GetCurrenciesAccepted().Count()));
        if (tTotalPrice > 0) {
            bool currencyResult = TraderXCurrencyService.GetInstance().RemoveMoneyAmountFromPlayer(player, tTotalPrice, npc.GetCurrenciesAccepted());
            GetTraderXLogger().LogInfo(string.Format("[TRANSACTION] Currency removal result: %1", currencyResult));
            if (!currencyResult) {
                // Rollback: remove all created items and restore stock
                for (int k = 0; k < allCreatedItems.Count(); k++) {
                    GetGame().ObjectDelete(allCreatedItems[k]);
                }
                if (!product.IsStockUnlimited()) {
                    TraderXProductStockRepository.IncreaseStock(transaction.GetProductId(), transaction.GetMultiplier());
                }
                if (preset && preset.attachments && preset.attachments.Count() > 0) {
                    RestorePresetAttachmentStock(preset);
                }
                GetTraderXLogger().LogError("[TRANSACTION] Transaction failed due to insufficient funds - rolled back");
                return TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), settings.purchaseFailedPrefix + settings.insufficientFunds);
            }
        }
        
        return TraderXTransactionResult.CreateSuccess(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), settings.purchaseSuccessful);
    }
    
    private TraderXTransactionResult ValidatePresetAttachmentStock(TraderXPreset preset, TraderXTransaction transaction)
    {
        TraderXDynamicTranslationSettings settings = TraderXDynamicTranslationRepository.GetSettings();
                
        for (int i = 0; i < preset.attachments.Count(); i++) {
            string attachmentId = preset.attachments.Get(i);
            TraderXProduct attachmentProduct = TraderXProductRepository.GetItemById(attachmentId);
            
            if (!attachmentProduct) {
                return TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), settings.purchaseFailedPrefix + string.Format(settings.attachmentProductNotFound, attachmentId));
            }
            
            if (!attachmentProduct.IsStockUnlimited()) {
                if (!TraderXProductStockRepository.HasStock(attachmentId)) {
                    return TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), settings.purchaseFailedPrefix + string.Format(settings.attachmentOutOfStock, attachmentId));
                }
            }
        }
        
        return TraderXTransactionResult.CreateSuccess(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), settings.presetValidationSuccessful);
        return TraderXTransactionResult.CreateSuccess(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), settings.presetValidationSuccessful);
    }
    
    private TraderXTransactionResult ApplyPresetToItem(ItemBase item, TraderXPreset preset, TraderXTransaction transaction)
    {
        TraderXDynamicTranslationSettings settings = TraderXDynamicTranslationRepository.GetSettings();
                
        for (int i = 0; i < preset.attachments.Count(); i++) {
            string attachmentId = preset.attachments.Get(i);
            TraderXProduct attachmentProduct = TraderXProductRepository.GetItemById(attachmentId);
            
            if (!attachmentProduct) {
                GetTraderXLogger().LogError("ApplyPresetToItem: Attachment product not found: " + attachmentId);
                return TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), string.Format(settings.failedToApplyPreset, attachmentId));
            }
            
            // Create and attach the attachment item using proper attachment method
            int attachmentQuantity = TraderXTradeQuantity.GetItemBuyQuantity(attachmentProduct.className, attachmentProduct.tradeQuantity);
            ItemBase attachmentItem = TraderXItemFactory.CreateAsAttachment(item, attachmentProduct.className, attachmentQuantity);
            if (!attachmentItem) {
                GetTraderXLogger().LogWarning("ApplyPresetToItem: Failed to create attachment, trying as regular item: " + attachmentId);
                
                // Fallback: Create as regular item in player inventory
                PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
                if (player) {
                    attachmentItem = TraderXItemFactory.CreateInInventory(player, attachmentProduct.className, attachmentQuantity);
                    if (!attachmentItem) {
                        GetTraderXLogger().LogError("ApplyPresetToItem: Failed to create attachment as regular item: " + attachmentId);
                        return TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), string.Format(settings.couldNotCreateAttachment, attachmentId));
                    }
                    GetTraderXLogger().LogDebug("ApplyPresetToItem: Successfully created attachment as regular item: " + attachmentId);
                } else {
                    GetTraderXLogger().LogError("ApplyPresetToItem: No player found for fallback item creation: " + attachmentId);
                    return TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), string.Format(settings.couldNotCreateAttachment, attachmentId));
                }
            }
            
            GetTraderXLogger().LogDebug("ApplyPresetToItem: Successfully attached " + attachmentId + " to item");
        }
        
        GetTraderXLogger().LogDebug("ApplyPresetToItem: Successfully applied all preset attachments");
        return TraderXTransactionResult.CreateSuccess(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), settings.presetAppliedSuccessfully);
    }
    
    private void UpdatePresetAttachmentStock(TraderXPreset preset)
    {
        GetTraderXLogger().LogDebug("UpdatePresetAttachmentStock: Updating stock for " + preset.attachments.Count() + " attachments");
        
        for (int i = 0; i < preset.attachments.Count(); i++) {
            string attachmentId = preset.attachments.Get(i);
            TraderXProduct attachmentProduct = TraderXProductRepository.GetItemById(attachmentId);
            
            if (attachmentProduct && !attachmentProduct.IsStockUnlimited()) {
                TraderXProductStockRepository.DecreaseStock(attachmentId);
                GetTraderXLogger().LogDebug("UpdatePresetAttachmentStock: Decreased stock for attachment: " + attachmentId);
            }
        }
    }
    
    private void RestorePresetAttachmentStock(TraderXPreset preset)
    {
        GetTraderXLogger().LogDebug("RestorePresetAttachmentStock: Restoring stock for " + preset.attachments.Count() + " attachments");
        
        for (int i = 0; i < preset.attachments.Count(); i++) {
            string attachmentId = preset.attachments.Get(i);
            TraderXProduct attachmentProduct = TraderXProductRepository.GetItemById(attachmentId);
            
            if (attachmentProduct && !attachmentProduct.IsStockUnlimited()) {
                TraderXProductStockRepository.IncreaseStock(attachmentId);
                GetTraderXLogger().LogDebug("RestorePresetAttachmentStock: Restored stock for attachment: " + attachmentId);
            }
        }
    }
    
    private TraderXTransactionResult ProcessSellTransaction(TraderXTransaction transaction, PlayerBase player)
    {
        TraderXDynamicTranslationSettings settings = TraderXDynamicTranslationRepository.GetSettings();
        
        // Vérification des prérequis
        if (!player) {
            return TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), settings.saleFailedPrefix + settings.invalidPlayer);
        }

        // Récupération du produit
        TraderXProduct product = TraderXProductRepository.GetItemById(transaction.GetProductId());
        if (!product) {
            return TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), settings.saleFailedPrefix + settings.productNotFound);
        }

        // Vérification de l'item à vendre et récupération de son état
        ItemBase itemToRemove = ItemBase.Cast(GetGame().GetObjectByNetworkId(transaction.GetNetworkId().GetLowId(), transaction.GetNetworkId().GetHighId()));
        
        if (!itemToRemove) {
            return TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), settings.saleFailedPrefix + settings.itemNotFoundInInventory);
        }
        
        // Get the actual item condition for accurate pricing
        int itemState = itemToRemove.GetHealthLevel();
        
        // Calculate dynamic sell price based on current stock, coefficient, and actual item condition
        TraderXPriceCalculation priceCalculation = TraderXPricingService.GetInstance().CalculateSellPrice(product, transaction.GetMultiplier(), itemState);
        int calculatedPrice = priceCalculation.GetCalculatedPrice();
        
        GetTraderXLogger().LogInfo(string.Format("[TRANSACTION] Sell price calculation - Product: %1, ItemState: %2, BasePrice: %3, CalculatedPrice: %4", product.className, itemState, product.sellPrice, calculatedPrice));
        
        // Price validation - reject transactions with -1 price (item cannot be sold)
        if (calculatedPrice < 0) {
            return TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), settings.saleFailedPrefix + settings.invalidPrice);
        }
        
        // Validate that the transaction price matches the calculated price (prevent client-side price manipulation)
        int transactionPrice = transaction.GetTotalPrice().GetAmount();
        if (transactionPrice != calculatedPrice) {
            GetTraderXLogger().LogWarning(string.Format("[TRANSACTION] Sell price mismatch - Expected: %1, Received: %2, Product: %3, ItemState: %4", calculatedPrice, transactionPrice, product.className, itemState));
            return TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), settings.saleFailedPrefix + settings.invalidPrice);
        }

        // Vehicle multiplier validation - vehicles can only be sold one at a time
        if (TraderXVehicleTransactionService.GetInstance().IsVehicleProduct(product.className)) {
            if (transaction.GetMultiplier() > 1) {
                return TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), settings.saleFailedPrefix + settings.vehicleSellMultipleQuantity);
            }
        }
        
        // Récupération des données complémentaires
        TraderXNpc npc = GetTraderXModule().GetSettings().GetNpcById(transaction.GetTraderId());
        
        if (!npc) {
            return TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), settings.saleFailedPrefix + settings.traderNotFound);
        }
        
        // Stock validation for sell transactions
        if (!product.IsStockUnlimited()) {
            if (!TraderXProductStockRepository.CanIncreaseStock(transaction.GetProductId(), product.maxStock)) {
                return TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), settings.saleFailedPrefix + settings.traderStockFull);
            }
        }
        
        // Retrait de l'item du joueur
        int remainingQty = TraderXInventoryManager.RemoveItem(player, itemToRemove, TraderXTradeQuantity.GetItemSellQuantity(product.className, product.tradeQuantity, TraderXQuantityManager.GetItemAmount(itemToRemove)) * transaction.GetMultiplier());
        
        // Mise à jour du stock
        if (!product.IsStockUnlimited()) {
            TraderXProductStockRepository.IncreaseStock(transaction.GetProductId(), transaction.GetMultiplier());
        }
        
        // Ajout de la monnaie au joueur
        TraderXCurrencyService.GetInstance().AddMoneyToPlayer(player, transaction.GetTotalPrice().GetAmount(), npc.GetCurrenciesAccepted());
        
        return TraderXTransactionResult.CreateSuccess(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), settings.saleSuccessful);
    }

    void ProcessTransactionQueue(float dt)
    {
        this.deltaTime += dt;
        if (this.deltaTime <= TRADERX_QUEUE_TRANSACTION_INTERVAL) 
            return;
        
        this.deltaTime = 0;
        if (!m_TransactionRequestQueue.HasNextQueue())
            return;
            
        // Process the first transaction request in the queue
        TraderXTransactionRequest request = m_TransactionRequestQueue.Peek();
        m_TransactionRequestQueue.DeQueue();

        GetTraderXLogger().LogDebug("ProcessTransactionQueue : " + request.GetTransactionCount());
        
        // Get debug service instance once for the entire method
        TraderXDebugModeService debugService = TraderXDebugModeService.GetInstance();
        TraderXDebugSettings debugSettings = GetTraderXModuleTest().GetDebugSettings();
        string playerId;
        
        if (!request || !request.IsValid())
        {

            GetTraderXLogger().LogDebug("ProcessTransactionQueue : Invalid request");
            
            // Capture failed transaction if debug mode is enabled and player is authorized admin
            if (debugService.IsDebugModeEnabled() && request && request.GetPlayer() && request.GetTransactionCollection())
            {
                playerId = request.GetPlayer().GetIdentity().GetPlainId();
                
                if (debugSettings.IsAuthorizedAdmin(playerId))
                {
                    GetTraderXLogger().LogDebug("CaptureFailedTransaction for authorized admin: " + playerId);
                    debugService.CaptureFailedTransaction(request.GetPlayer(), request.GetTransactionCollection(), request.GetNpcId(), "Invalid transaction request");
                }
                else
                {
                    GetTraderXLogger().LogDebug("Failed transaction capture skipped - player not authorized: " + playerId);
                }
            }
            return;
        }

        GetTraderXLogger().LogDebug("ProcessTransactionQueue : " + request.GetTransactionCollection().ToStringFormatted());
        
        // Begin transaction capture if debug mode is enabled and player is authorized admin
        if (debugService.IsDebugModeEnabled())
        {
            playerId = request.GetPlayer().GetIdentity().GetPlainId();
            if (debugSettings.IsAuthorizedAdmin(playerId))
            {
                GetTraderXLogger().LogDebug("BeginTransactionCapture for authorized admin: " + playerId);
                debugService.BeginTransactionCapture(request.GetPlayer(), request.GetTransactionCollection(), request.GetNpcId(), "Admin transaction request");
            }
            else
            {
                GetTraderXLogger().LogDebug("Transaction capture skipped - player not authorized: " + playerId);
            }
        }
        
        // Process all transactions in the request using the coordinator
        array<ref TraderXTransactionResult> results =  TraderXTransactionCoordinator.GetInstance().ProcessTransactionBatch(request.GetTransactionCollection(), request.GetPlayer());
        
        // Create the result collection
        TraderXTransactionResultCollection resultCollection = TraderXTransactionResultCollection.Create(request.GetSteamId(), results);
        
        // Complete transaction capture if debug mode is enabled and player is authorized admin
        if (debugService.IsDebugModeEnabled())
        {
            playerId = request.GetPlayer().GetIdentity().GetPlainId();
            
            if (debugSettings.IsAuthorizedAdmin(playerId))
            {
                GetTraderXLogger().LogDebug("CompleteTransactionCapture for authorized admin: " + playerId);
                bool wasSuccessful = !resultCollection.HasErrors();
                debugService.CompleteTransactionCapture(request.GetPlayer(), resultCollection, wasSuccessful);
            }
        }
        
        // Send the response to the client
        SendTransactionResponse(resultCollection, request.GetPlayer().GetIdentity());
    }

    void ProcessTransactionResponseQueue(float deltaTime)
    {
        this.deltaTime += deltaTime;
        if (this.deltaTime <= TRADERX_QUEUE_TRANSACTION_INTERVAL) 
            return;
        
        this.deltaTime = 0;
        if (m_TransactionResponseQueue.Count() == 0)
            return;
            
        // Traiter la première réponse de transaction
        TraderXTransactionResultCollection resultCollection = m_TransactionResponseQueue.Peek();
        m_TransactionResponseQueue.DeQueue();
        
        if (!resultCollection)
            return;

        GetTraderXLogger().LogDebug("[TEST] TraderXTransactionService::ProcessTransactionResponseQueue");
        GetTraderXLogger().LogDebug("[TEST] TraderXTransactionService::ProcessTransactionResponseQueue : " + resultCollection.ToStringFormatted());
            
        // Notifier le service de trading des résultats
        TraderXTradingService.GetInstance().OnTraderXResponseReceived(ETraderXResponse.TRANSACTIONS, resultCollection);
    }
    
    private void SendTransactionResponse(TraderXTransactionResultCollection resultCollection, PlayerIdentity playerIdentity)
    {
        if (!resultCollection || !playerIdentity)
            return;
            
        GetRPCManager().SendRPC("TraderX", "OnTransactionsResponse", new Param1<TraderXTransactionResultCollection>(resultCollection), true, playerIdentity);
        
        // Send updated stock data to client
        SendUpdatedStockToClient(resultCollection, playerIdentity);
    }
    
    private void SendUpdatedStockToClient(TraderXTransactionResultCollection resultCollection, PlayerIdentity playerIdentity)
    {
        // Collect unique product IDs from all transactions
        array<string> updatedProductIds = new array<string>();
        array<ref TraderXTransactionResult> results = resultCollection.GetTransactionResults();
        
        foreach (TraderXTransactionResult result : results)
        {
            string productId = result.GetProductId();
            if (updatedProductIds.Find(productId) == -1) // Avoid duplicates
            {
                updatedProductIds.Insert(productId);
            }
        }
        
        // Get current stock for these products
        array<ref TraderXProductStock> updatedStocks = new array<ref TraderXProductStock>();
        foreach (string prodId : updatedProductIds)
        {
            TraderXProductStock stock = TraderXProductStockRepository.GetStockByProductId(prodId);
            if (stock)
            {
                updatedStocks.Insert(stock);
            }
        }
        
        // Send stock update to client
        if (updatedStocks.Count() > 0)
        {
            GetRPCManager().SendRPC("TraderX", "OnStockUpdateResponse", new Param1<array<ref TraderXProductStock>>(updatedStocks), true, playerIdentity);
        }
    }

    //RPCs 
    void GetTransactionsRequest(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if(type != CallType.Server)
            return;

        Param2<TraderXTransactionCollection, int> data;
        if(!ctx.Read(data)){
            return;
        }

        PlayerBase player = TraderXHelper.GetPlayerByIdentity(sender);
        if (!player) {
            GetTraderXLogger().LogError("GetTransactionsRequest: Player not found for identity");
            return;
        }

        int npcId = data.param2;
        TraderXTransactionCollection transactionCollection = data.param1;
        if (!transactionCollection || transactionCollection.IsEmpty()) {
            GetTraderXLogger().LogError("GetTransactionsRequest: Invalid or empty transaction collection");
            return;
        }

        // Créer une requête de transaction avec le wrapper
        TraderXTransactionRequest request = TraderXTransactionRequest.Create(sender.GetPlainId(), player, transactionCollection, npcId);
        
        // Ajouter à la file d'attente
        m_TransactionRequestQueue.EnQueue(request);
        
        GetTraderXLogger().LogDebug(string.Format("Transaction request queued for player %1 with %2 transactions", sender.GetPlainId(), transactionCollection.GetCount()));
    }

    void OnTransactionsResponse(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if(type != CallType.Client)
            return;

        GetTraderXLogger().LogDebug("OnTransactionsResponse");

        Param1<TraderXTransactionResultCollection> data;
        if(!ctx.Read(data)){
            GetTraderXLogger().LogError("OnTransactionsResponse: Failed to read transaction result collection");
            return;
        }

        TraderXTransactionResultCollection resultCollection = data.param1;
        if (!resultCollection) {
            GetTraderXLogger().LogError("OnTransactionsResponse: Invalid result collection");
            return;
        }
        
        // Ajouter à la file d'attente des réponses
        m_TransactionResponseQueue.EnQueue(resultCollection);
        
        GetTraderXLogger().LogDebug(string.Format("Transaction response queued for player %1 with %2 results", resultCollection.GetSteamId(), resultCollection.Count()));
    }
    
    void OnStockUpdateResponse(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if(type != CallType.Client)
            return;

        Param1<array<ref TraderXProductStock>> data;
        if(!ctx.Read(data)){
            return;
        }

        array<ref TraderXProductStock> updatedStocks = data.param1;
        if (!updatedStocks) {
            GetTraderXLogger().LogError("OnStockUpdateResponse: Invalid stock data");
            return;
        }

        GetTraderXLogger().LogDebug("OnStockUpdateResponse: Received " + updatedStocks.Count() + " stock updates");
        
        // Update client-side stock repository
        TraderXProductStockRepository.RefStockToTraderXProduct(updatedStocks);
        
        // Notify trading service that stock has been updated
        TraderXTradingService.GetInstance().OnTraderXResponseReceived(ETraderXResponse.ALL_STOCK_RECEIVED);
    }
}