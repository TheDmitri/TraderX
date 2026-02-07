enum ETraderXTransactionResultType
{
    SUCCESS,
    FAILURE
}

class TraderXTransactionResult
{
    //TODO: remove finish this
    ref TraderXTransactionId transactionId;
    string productId;
    TraderXTransactionType transactionType;
    ETraderXTransactionResultType resultType;
    string message;
    
    void TraderXTransactionResult(TraderXTransactionId id, string prodId, TraderXTransactionType transType, ETraderXTransactionResultType transResultType, string msg)
    {
        transactionId = id;
        this.productId = prodId;
        this.transactionType = transType;
        resultType = transResultType;
        message = msg;
    }
    
    static TraderXTransactionResult CreateSuccess(TraderXTransactionId transId, string prodId, TraderXTransactionType transType, string msg = "Transaction completed successfully")
    {
        return new TraderXTransactionResult(transId, prodId, transType, ETraderXTransactionResultType.SUCCESS, msg);
    }
    
    static TraderXTransactionResult CreateFailure(TraderXTransactionId transId, string prodId, TraderXTransactionType transType, string msg)
    {
        return new TraderXTransactionResult(transId, prodId, transType, ETraderXTransactionResultType.FAILURE, msg);
    }
    
    // Getters
    ETraderXTransactionResultType GetResultType() { return resultType; }
    string GetMessage() { return message; }
    TraderXTransactionId GetTransactionId() { return transactionId; }
    string GetProductId() { return productId; }
    TraderXTransactionType GetTransactionType() { return transactionType; }
    
    // Query methods
    bool IsSuccess() { return resultType == ETraderXTransactionResultType.SUCCESS; }
    bool IsFailure() { return resultType == ETraderXTransactionResultType.FAILURE; }

    string ToStringFormatted()
    {
        return "TransactionId: " + transactionId + " ProductId: " + productId + " TransactionType: " + transactionType + " ResultType: " + resultType + " Message: " + message + "\n";
    }
}