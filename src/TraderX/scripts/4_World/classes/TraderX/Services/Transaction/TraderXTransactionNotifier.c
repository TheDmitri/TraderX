class TraderXTransactionNotifier
{
    private static ref TraderXTransactionNotifier m_Instance;
    
    static TraderXTransactionNotifier GetInstance()
    {
        if (!m_Instance)
        {
            m_Instance = new TraderXTransactionNotifier();
        }
        return m_Instance;
    }
    
    void TraderXTransactionNotifier()
    {
        TraderXTradingService.Event_OnTraderXResponseReceived.Insert(OnTraderXResponseReceived);
    }
    
    // Format notification title based on transaction results
    string FormatNotificationTitle(TraderXTransactionResultCollection results)
    {
        TraderXDynamicTranslationSettings settings = TraderXDynamicTranslationRepository.GetSettings();
        
        if (results.Count() > 1)
        {
            return settings.bulkTransactionResult;
        }
        else if (results.Count() == 1)
        {
            auto result = results.GetTransactionResults()[0];
            return Ternary<string>.If(result.IsSuccess(), "#tpm_transaction_successful", "#tpm_transaction_failed");
        }
        return "#tpm_transaction_notification";
    }
    
    // Format notification content based on transaction results
    string FormatNotificationContent(TraderXTransactionResultCollection results)
    {
        TraderXDynamicTranslationSettings settings = TraderXDynamicTranslationRepository.GetSettings();
        
        if (results.Count() > 1)
        {
            int successCount = 0;
            foreach (auto result : results.GetTransactionResults())
            {
                if (result.IsSuccess()) successCount++;
            }
            return string.Format(settings.bulkTransactionSummary, successCount, results.Count());
        }
        else if (results.Count() == 1)
        {
            auto rResult = results.GetTransactionResults()[0];
            if (!rResult) {
                GetTraderXLogger().LogError("FormatNotificationContent - Transaction result is null");
                return "Transaction failed - invalid result";
            }
            
            TraderXProduct product = TraderXProductRepository.GetItemById(rResult.GetProductId());
            if (!product) {
                GetTraderXLogger().LogError(string.Format("FormatNotificationContent - Product not found for ID: %1", rResult.GetProductId()));
                return "Transaction failed - product not found";
            }
            
            string itemName = product.GetDisplayName();
            TraderXTransactionType transactionType = rResult.GetTransactionType();
            if (!transactionType) {
                GetTraderXLogger().LogError("FormatNotificationContent - Transaction type is null");
                return string.Format("Transaction completed for %1", itemName);
            }
            
            if (rResult.IsSuccess())
            {
                return string.Format(Ternary<string>.If(transactionType.IsSell(), settings.successfullySold, settings.successfullyBought), itemName);
            }
            else
            {
                return string.Format(Ternary<string>.If(transactionType.IsSell(), settings.failedToSell, settings.failedToBuy), itemName, rResult.GetMessage());
            }
        }
        return "";
    }
    
    // Get notification type (SUCCESS, WARNING, ERROR)
    int GetNotificationType(TraderXTransactionResultCollection results)
    {
        if (results.Count() == 1)
        {
            auto rResult = results.GetTransactionResults()[0];
            return Ternary<int>.If(rResult.IsSuccess(), ENotificationType.SUCCESS, ENotificationType.ERROR);
        }
        
        bool hasSuccess = false;
        bool hasError = false;
        
        for(int i = 0; i < results.GetTransactionResults().Count(); i++)
        {
            TraderXTransactionResult result = results.GetTransactionResults()[i];
            if (result.IsSuccess()) hasSuccess = true;
            else hasError = true;
        }
        
        if (hasSuccess && hasError) return ENotificationType.WARNING;
        if (hasError) return ENotificationType.ERROR;
        return ENotificationType.SUCCESS;
    }
    
    // Main method to show transaction notification
    void ShowTransactionNotification(TraderXTransactionResultCollection results)
    {
        GetTraderXLogger().LogInfo("[TEST] TraderXTransactionNotifier::ShowTransactionNotification");
        if (!results || results.Count() == 0) return;
        
        string title = FormatNotificationTitle(results);
        string content = FormatNotificationContent(results);
        int type = GetNotificationType(results);
        
        // Use the notification service to display the notification
        TraderXNotificationService.GetInstance().ShowTransactionNotification(type, 5, title, content, results.Count() > 1, "#tpm_show_details", GetNotificationDetails(results));
    }
    
    // Get formatted details for the notification
    private array<ref TraderXNotificationDetails> GetNotificationDetails(TraderXTransactionResultCollection results)
    {
        array<ref TraderXNotificationDetails> details = new array<ref TraderXNotificationDetails>();
        
        if (results.Count() <= 1) return details;
        
        for(int i = 0; i < results.GetTransactionResults().Count(); i++)
        {
            TraderXTransactionResult result = results.GetTransactionResults()[i];
            string itemName = TraderXProductRepository.GetItemById(result.GetProductId()).GetDisplayName();
            TraderXDynamicTranslationSettings settings = TraderXDynamicTranslationRepository.GetSettings();
            string status = Ternary<string>.If(result.IsSuccess(), settings.completed, string.Format(settings.failedWithReason, result.GetMessage()));
            
            TraderXNotificationDetails detail = new TraderXNotificationDetails(result.IsSuccess(), itemName, status);
            details.Insert(detail);
        }
        
        return details;
    }

    void OnTraderXResponseReceived(int response, TraderXTransactionResultCollection transactionResultCollection)
    {
        GetTraderXLogger().LogInfo("[TEST] TraderXTransactionNotifier::OnTraderXResponseReceived");
        if(response == ETraderXResponse.TRANSACTIONS && transactionResultCollection.Count() > 0){
            ShowTransactionNotification(transactionResultCollection);
        }
    }
}