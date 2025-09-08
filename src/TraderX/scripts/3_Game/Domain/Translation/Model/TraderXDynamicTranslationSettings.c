class TraderXDynamicTranslationSettings
{
    string version = TRADERX_CURRENT_VERSION;
    
    // Transaction notification messages with placeholders
    string bulkTransactionResult = "Bulk Transaction Result";
    string bulkTransactionSummary = "%1 out of %2 transactions were successful";
    string successfullyBought = "You successfully bought %1";
    string successfullySold = "You successfully sold %1";
    string failedToBuy = "Failed to buy %1: %2";
    string failedToSell = "Failed to sell %1: %2";
    string completed = "Completed";
    string failedWithReason = "Failed: %1";
    
    // Transaction error messages
    string invalidTransaction = "Invalid transaction";
    string invalidPlayer = "Invalid player";
    string productNotFound = "Product not found";
    string vehicleMultipleQuantity = "Vehicles can only be purchased one at a time";
    string insufficientStock = "Insufficient stock for quantity requested";
    string invalidPresetDetected = "Security: Invalid preset detected";
    string couldNotCreateWeapon = "Could not create weapon %1";
    string couldNotCreateItem = "Could not create item";
    string insufficientFunds = "Insufficient funds";
    string purchaseSuccessful = "Purchase successful";
    string attachmentProductNotFound = "Attachment product not found: %1";
    string attachmentOutOfStock = "Attachment out of stock: %1";
    string presetValidationSuccessful = "Preset validation successful";
    string failedToApplyPreset = "Failed to apply preset: Attachment not found: %1";
    string couldNotCreateAttachment = "Failed to apply preset: Could not create attachment: %1";
    string presetAppliedSuccessfully = "Preset applied successfully";
    string vehicleSellMultipleQuantity = "Vehicles can only be sold one at a time";
    string itemNotFoundInInventory = "Item not found in inventory";
    string traderNotFound = "Trader not found";
    string traderStockFull = "Trader stock is full";
    string saleSuccessful = "Sale successful";
    string invalidPrice = "Invalid price - transactions with negative prices are not allowed";
    
    // Transaction error prefixes
    string purchaseFailedPrefix = "Purchase failed: ";
    string saleFailedPrefix = "Sale failed: ";
    
    void TraderXDynamicTranslationSettings()
    {
        // Constructor - default values already set above
    }
}
