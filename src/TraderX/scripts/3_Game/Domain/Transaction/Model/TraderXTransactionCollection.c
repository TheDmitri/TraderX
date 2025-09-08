class TraderXTransactionCollection
{
    private ref array<ref TraderXTransaction> transactions;
    
    void TraderXTransactionCollection()
    {
        transactions = new array<ref TraderXTransaction>();
    }
    
    static TraderXTransactionCollection CreateEmpty()
    {
        return new TraderXTransactionCollection();
    }
    
    void AddTransaction(TraderXTransaction transaction)
    {
        // Ensure transactions array is initialized
        if (!transactions) {
            GetTraderXLogger().LogWarning("AddTransaction - transactions array was null, reinitializing");
            transactions = new array<ref TraderXTransaction>();
        }
        
        GetTraderXLogger().LogDebug(string.Format("AddTransaction - transaction null: %1", transaction == null));
        if (transaction) {
            GetTraderXLogger().LogDebug(string.Format("AddTransaction - transaction.IsValid(): %1", transaction.IsValid()));
            if (transaction.IsValid()) {
                transactions.Insert(transaction);
                GetTraderXLogger().LogDebug(string.Format("AddTransaction - Transaction added, collection count: %1", transactions.Count()));
            } else {
                GetTraderXLogger().LogError("AddTransaction - Transaction is invalid, not adding to collection");
            }
        } else {
            GetTraderXLogger().LogError("AddTransaction - Transaction is null");
        }
    }
    
    void RemoveTransaction(TraderXTransactionId transactionId)
    {
        if (!transactions) {
            GetTraderXLogger().LogWarning("RemoveTransaction - transactions array was null, reinitializing");
            transactions = new array<ref TraderXTransaction>();
            return;
        }
        
        for (int i = transactions.Count() - 1; i >= 0; i--)
        {
            if (transactions[i].GetTransactionId().Equals(transactionId))
            {
                transactions.RemoveOrdered(i);
                break;
            }
        }
    }
    
    TraderXTransaction GetTransaction(TraderXTransactionId transactionId)
    {
        if (!transactions) {
            GetTraderXLogger().LogWarning("GetTransaction - transactions array was null, reinitializing");
            transactions = new array<ref TraderXTransaction>();
            return null;
        }
        
        foreach (TraderXTransaction transaction : transactions)
        {
            if (transaction.GetTransactionId().Equals(transactionId))
                return transaction;
        }
        return null;
    }
    
    array<ref TraderXTransaction> GetAllTransactions()
    {
        if (!transactions) {
            GetTraderXLogger().LogWarning("GetAllTransactions - transactions array was null, reinitializing");
            transactions = new array<ref TraderXTransaction>();
        }
        return transactions;
    }
    
    array<ref TraderXTransaction> GetBuyTransactions()
    {
        if (!transactions) {
            GetTraderXLogger().LogWarning("GetBuyTransactions - transactions array was null, reinitializing");
            transactions = new array<ref TraderXTransaction>();
        }
        
        array<ref TraderXTransaction> buyTransactions = new array<ref TraderXTransaction>();
        foreach (TraderXTransaction transaction : transactions)
        {
            if (transaction.IsBuy())
                buyTransactions.Insert(transaction);
        }
        return buyTransactions;
    }
    
    array<ref TraderXTransaction> GetSellTransactions()
    {
        if (!transactions) {
            GetTraderXLogger().LogWarning("GetSellTransactions - transactions array was null, reinitializing");
            transactions = new array<ref TraderXTransaction>();
        }
        
        array<ref TraderXTransaction> sellTransactions = new array<ref TraderXTransaction>();
        foreach (TraderXTransaction transaction : transactions)
        {
            if (transaction.IsSell())
                sellTransactions.Insert(transaction);
        }
        return sellTransactions;
    }
    
    int GetCount()
    {
        if (!transactions) {
            GetTraderXLogger().LogWarning("GetCount - transactions array was null, reinitializing");
            transactions = new array<ref TraderXTransaction>();
        }
        return transactions.Count();
    }
    
    bool IsEmpty()
    {
        if (!transactions) {
            GetTraderXLogger().LogWarning("IsEmpty - transactions array was null, reinitializing");
            transactions = new array<ref TraderXTransaction>();
        }
        return transactions.Count() == 0;
    }
    
    TraderXTransactionPrice GetTotalPrice()
    {
        if (!transactions) {
            GetTraderXLogger().LogWarning("GetTotalPrice - transactions array was null, reinitializing");
            transactions = new array<ref TraderXTransaction>();
        }
        
        TraderXTransactionPrice total = TraderXTransactionPrice.CreateZero();
        foreach (TraderXTransaction transaction : transactions)
        {
            total = total.Add(transaction.GetTotalPrice());
        }
        return total;
    }
    
    void Clear()
    {
        if (!transactions) {
            GetTraderXLogger().LogWarning("Clear - transactions array was null, reinitializing");
            transactions = new array<ref TraderXTransaction>();
        }
        transactions.Clear();
    }
    
    // Tri par profondeur (depth) pour les transactions de vente
    void SortByDepth()
    {
        if (!transactions) {
            GetTraderXLogger().LogWarning("SortByDepth - transactions array was null, reinitializing");
            transactions = new array<ref TraderXTransaction>();
            return;
        }
        
        // Tri simple par insertion basé sur la profondeur
        for (int i = 1; i < transactions.Count(); i++)
        {
            TraderXTransaction current = transactions[i];
            int j = i - 1;
            
            while (j >= 0 && transactions[j].GetDepth() < current.GetDepth())
            {
                transactions[j + 1] = transactions[j];
                j--;
            }
            transactions[j + 1] = current;
        }
    }

    string ToStringFormatted()
    {
        string result = "";
        result += "TraderXTransactionCollection {\n";
        result += "  count: " + transactions.Count().ToString() + "\n";
        result += "  transactions: [\n";
        
        for (int i = 0; i < transactions.Count(); i++)
        {
            TraderXTransaction transaction = transactions.Get(i);
            if (transaction)
            {
                // Add each transaction with proper indentation
                string transactionStr = transaction.ToStringFormatted();
                TStringArray lines = new TStringArray();
                transactionStr.Split("\n", lines);
                for (int j = 0; j < lines.Count(); j++)
                {
                    result += "    " + lines.Get(j);
                    if (j < lines.Count() - 1) result += "\n";
                }
                
                // Add comma if not the last item
                if (i < transactions.Count() - 1)
                {
                    result += ",";
                }
                result += "\n";
            }
        }
        
        result += "  ]\n";
        
        result += "}";
        return result;
    }
}