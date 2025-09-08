/*
=============================================================================
     Program: TraderX Preset Validation Service
     File: TraderXPresetValidationService.c
     Author: Security Enhancement
     Version: v1.0
     Date: 02/09/2025

     Description: Server-side validation service for client presets to prevent
                  security exploits where players can manually edit preset files
                  to obtain items not sold by specific traders.
=============================================================================
*/

class TraderXPresetValidationService
{
    static ref TraderXPresetValidationService m_instance;

    static TraderXPresetValidationService GetInstance()
    {
        if (!m_instance)
        {
            m_instance = new TraderXPresetValidationService();
        }
        return m_instance;
    }

    // Main validation method for preset transactions
    TraderXTransactionResult ValidatePresetForTrader(PlayerBase player, TraderXPreset preset, string traderId, TraderXTransaction transaction)
    {
        if (!preset || !preset.attachments)
        {
            return TraderXTransactionResult.CreateSuccess(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), "No preset attachments to validate");
        }

        // Get trader configuration
        TraderXNpc npc = GetTraderXModule().GetSettings().GetNpcById(traderId.ToInt());
        if (!npc)
        {
            GetTraderXLogger().LogError("ValidatePresetForTrader: Trader not found: " + traderId);
            return TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), "Security: Invalid trader");
        }

        // Validate main product is available from this trader
        TraderXTransactionResult mainProductValidation = ValidateProductAvailableFromTrader(preset.productId, npc, transaction);
        if (!mainProductValidation.IsSuccess())
        {
            GetTraderXLogger().LogWarning(string.Format("Security Alert: Player attempted to purchase unavailable main product %1 from trader %2", preset.productId, traderId));
            return mainProductValidation;
        }

        // Validate each attachment is available from this trader
        for (int i = 0; i < preset.attachments.Count(); i++)
        {
            string attachmentId = preset.attachments.Get(i);
            TraderXTransactionResult attachmentValidation = ValidateProductAvailableFromTrader(attachmentId, npc, transaction);
            
            if (!attachmentValidation.IsSuccess())
            {
                GetTraderXLogger().LogWarning(string.Format("Security Alert: Player attempted to purchase unavailable attachment %1 from trader %2", attachmentId, traderId));
                return TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), "Security: Attachment not available from this trader: " + attachmentId);
            }
        }

        // Validate player can afford the total cost
        TraderXTransactionResult costValidation = ValidatePresetTotalCost(player, preset, npc, transaction);
        if (!costValidation.IsSuccess())
        {
            return costValidation;
        }

        GetTraderXLogger().LogDebug("ValidatePresetForTrader: All preset items validated successfully for trader " + traderId);
        return TraderXTransactionResult.CreateSuccess(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), "Preset validation successful");
    }

    // Validate if a specific product is available from a trader
    private TraderXTransactionResult ValidateProductAvailableFromTrader(string productId, TraderXNpc npc, TraderXTransaction transaction)
    {
        // Check if product exists globally first
        TraderXProduct product = TraderXProductRepository.GetItemById(productId);
        if (!product)
        {
            return TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), "Security: Product not found: " + productId);
        }

        // Check if trader sells this product through their categories
        array<string> traderCategories = npc.GetCategories();
        for (int i = 0; i < traderCategories.Count(); i++)
        {
            TraderXCategory category = TraderXCategoryRepository.GetCategoryById(traderCategories.Get(i));
            if (category && category.ContainsProduct(productId))
            {
                return TraderXTransactionResult.CreateSuccess(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), "Product available from trader");
            }
        }

        // Product not found in any of trader's categories
        return TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), "Security: Product not sold by this trader: " + productId);
    }

    // Additional validation for preset integrity (detect tampering)
    bool ValidatePresetIntegrity(TraderXPreset preset)
    {
        if (!preset)
            return false;

        // Basic integrity checks
        if (preset.presetId == "" || preset.presetName == "" || preset.productId == "")
        {
            GetTraderXLogger().LogWarning("Security Alert: Preset with missing required fields detected");
            return false;
        }



        return true;
    }

    // Log security events for monitoring
    void LogSecurityEvent(string eventType, string details, PlayerIdentity player = null)
    {
        string playerInfo = "";
        if (player)
        {
            playerInfo = string.Format(" Player: %1 (%2)", player.GetName(), player.GetId());
        }

        GetTraderXLogger().LogWarning(string.Format("[SECURITY] %1: %2%3", eventType, details, playerInfo));
    }

    // Validate total cost of preset against player's available funds
    private TraderXTransactionResult ValidatePresetTotalCost(PlayerBase player, TraderXPreset preset, TraderXNpc npc, TraderXTransaction transaction)
    {
        int totalCost = 0;
        
        // Use the same progressive pricing calculation as the transaction service
        if (transaction.GetMultiplier() > 1) {
            totalCost = TraderXPresetsService.GetInstance().CalculateProgressivePresetPrice(preset, transaction.GetMultiplier());
        } else {
            totalCost = TraderXPresetsService.GetInstance().CalculateTotalPricePreset(preset);
        }
        
        // Get player's available money for this trader's accepted currencies
        if (!player)
            return TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), "Security: Invalid player");
        
        TStringArray acceptedCurrencies = npc.GetCurrenciesAccepted();
        int playerMoney = TraderXCurrencyService.GetInstance().GetPlayerMoneyFromAllCurrency(player, acceptedCurrencies);
        
        if (playerMoney < totalCost)
        {
            GetTraderXLogger().LogWarning(string.Format("Security Alert: Player attempted preset purchase without sufficient funds. Cost: %1, Available: %2", totalCost, playerMoney));
            return TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), "Security: Insufficient funds for preset purchase");
        }
        
        return TraderXTransactionResult.CreateSuccess(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), "Cost validation successful");
    }
}
