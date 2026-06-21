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
    
    // Map a transaction result set to a toast type key (TXNOTIF_*). Single results
    // distinguish purchase vs sale; multi results collapse to info/partial/error.
    private string ResolveToastType(TraderXTransactionResultCollection results)
    {
        if (results.Count() == 1)
        {
            TraderXTransactionResult r = results.GetTransactionResults()[0];
            if (!r || !r.IsSuccess())
                return TXNOTIF_ERROR;

            TraderXTransactionType tt = r.GetTransactionType();
            if (tt && tt.IsSell())
                return TXNOTIF_SALE;
            return TXNOTIF_PURCHASE;
        }

        int type = GetNotificationType(results);
        if (type == ENotificationType.ERROR)   return TXNOTIF_ERROR;
        if (type == ENotificationType.WARNING) return TXNOTIF_WARNING;
        return TXNOTIF_INFO;
    }

    // One breakdown line per independent transaction, for the expandable detail
    // panel of a bulk toast: "Sold Ammo Box" / "Bought M4" / "Failed Canteen".
    private void BuildDetailLines(TraderXTransactionResultCollection results, array<string> outDetails)
    {
        foreach (TraderXTransactionResult r : results.GetTransactionResults())
        {
            if (!r) continue;

            TraderXProduct product = TraderXProductRepository.GetItemById(r.GetProductId());
            string itemName = r.GetProductId();
            if (product) itemName = product.GetDisplayName();

            bool isSell = false;
            TraderXTransactionType tt = r.GetTransactionType();
            if (tt) isSell = tt.IsSell();

            string line;
            if (r.IsSuccess())
                line = string.Format("%1 %2", Ternary<string>.If(isSell, "Sold", "Bought"), itemName);
            else
                line = string.Format("Failed %1", itemName);

            outDetails.Insert(line);
        }
    }

    // Main method to show transaction notification — routes to the HUD toast runner.
    void ShowTransactionNotification(TraderXTransactionResultCollection results)
    {
        if (!results || results.Count() == 0) return;

        string title = FormatNotificationTitle(results);
        string content = FormatNotificationContent(results);
        string toastType = ResolveToastType(results);

        // Bulk trades carry a per-transaction breakdown the card can expand to show.
        array<string> details = new array<string>();
        if (results.Count() > 1)
            BuildDetailLines(results, details);

        GetTraderXToastService().ShowToast(toastType, title, content, "", details);
    }

    void OnTraderXResponseReceived(int response, TraderXTransactionResultCollection transactionResultCollection)
    {
        if(response == ETraderXResponse.TRANSACTIONS && transactionResultCollection.Count() > 0){
            ShowTransactionNotification(transactionResultCollection);
        }
    }
}