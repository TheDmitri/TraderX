class TraderXTransactionCoordinator
{
    static ref TraderXTransactionCoordinator m_instance;
    private ref TraderXTransactionService transactionService;
    private ref TraderXTransactionValidator validator;
    
    static TraderXTransactionCoordinator GetInstance()
    {
        if (!m_instance)
            m_instance = new TraderXTransactionCoordinator();
        return m_instance;
    }
    
    void TraderXTransactionCoordinator()
    {
        transactionService = TraderXTransactionService.GetInstance();
        validator = new TraderXTransactionValidator();
    }
    
    array<ref TraderXTransactionResult> ProcessTransactionBatch(TraderXTransactionCollection transactions, PlayerBase player)
    {
        GetTraderXLogger().LogDebug("ProcessTransactionBatch");
        array<ref TraderXTransactionResult> results = new array<ref TraderXTransactionResult>();
        
        if (!transactions || transactions.IsEmpty())
        {
            GetTraderXLogger().LogDebug("ProcessTransactionBatch : No transactions");
            return results;
        }

        GetTraderXLogger().LogDebug("ProcessTransactionBatch : " + transactions.ToStringFormatted());
        
        // Tri des transactions de vente par profondeur (plus profond en premier)
        transactions.SortByDepth();

        GetTraderXLogger().LogDebug("ProcessTransactionBatch : " + transactions.ToStringFormatted());
        
        // Traitement séquentiel avec validation individuelle
        for(int i = 0; i < transactions.GetAllTransactions().Count(); i++)
        {
            TraderXTransaction transaction = transactions.GetAllTransactions().Get(i);
            GetTraderXLogger().LogDebug("ProcessTransactionBatch : " + transaction.GetTransactionId() + " " + transaction.GetProductId());
            // Validation individuelle de chaque transaction
            string validationError;
            if (!validator.ValidateTransaction(transaction, player, validationError))
            {
                GetTraderXLogger().LogDebug("ProcessTransactionBatch : Invalid transaction");
                // Transaction invalide : ajouter un résultat d'échec
                results.Insert(TraderXTransactionResult.CreateFailure(transaction.GetTransactionId(), transaction.GetProductId(), transaction.GetTransactionType(), validationError));
                continue; // Passer à la transaction suivante
            }
            
            GetTraderXLogger().LogDebug("ProcessTransactionBatch : Valid transaction");
            // Transaction valide : traiter normalement
            TraderXTransactionResult result = transactionService.ProcessTransaction(transaction, player);
            results.Insert(result);
        }
        
        return results;
    }
    
    private bool ShouldStopOnFailure(TraderXTransaction transaction)
    {
        // Logique pour déterminer si on doit arrêter le batch en cas d'échec
        // Par exemple, arrêter pour les transactions critiques
        return false; // Pour l'instant, continuer même en cas d'échec
    }
}