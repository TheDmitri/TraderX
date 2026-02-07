class TraderXTransactionResultCollection
{
    string steamId;
    ref array<ref TraderXTransactionResult> transactionResults;

    void TraderXTransactionResultCollection(string steamId, array<ref TraderXTransactionResult> results = null)
    {
        this.steamId = steamId;
        if(!results){
            transactionResults = new array<ref TraderXTransactionResult>();
        }
        else
        {
            this.transactionResults = results;
        }
    }

    void ~TraderXTransactionResultCollection()
    {
        Clear();
    }

    static TraderXTransactionResultCollection Create(string id, array<ref TraderXTransactionResult> results)
    {
        return new TraderXTransactionResultCollection(id, results);
    }

    string GetSteamId()
    {
        return steamId;
    }

    array<ref TraderXTransactionResult> GetTransactionResults()
    {
        return transactionResults;
    }

    bool HasErrors()
    {
        foreach (auto result : transactionResults)
        {
            if (!result.IsSuccess())
            {
                return true;
            }
        }
        return false;
    }

    int Count()
    {
        return transactionResults.Count();
    }

    array<string> GetAllErrorMessages()
    {
        array<string> errors = new array<string>();
        foreach (auto result : transactionResults)
        {
            if (!result.IsSuccess())
            {
                errors.Insert(result.GetMessage());
            }
        }
        return errors;
    }

    void AddTransactionResult(TraderXTransactionResult transactionResult)
    {
        transactionResults.Insert(transactionResult);
    }

    void Clear()
    {
        transactionResults.Clear();
    }

    string ToStringFormatted()
    {
        string result = "";
        foreach (auto transactionResult : transactionResults)
        {
            result += transactionResult.ToStringFormatted();
        }
        return result;
    }
}
