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
    ref TraderXTransactionType transactionType;
    ETraderXTransactionResultType resultType;
    string message;
    // Set on the specific FAILURE where a BUY was rejected only because it wouldn't fit in
    // the player's inventory (ground-drop not yet confirmed). The client uses this to show a
    // "drop on ground?" confirm prompt instead of a plain failure toast. Serialized.
    bool needsSpaceConfirm;

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

    // A failure that the player can retry by confirming a ground-drop (inventory was full).
    static TraderXTransactionResult CreateNoSpace(TraderXTransactionId transId, string prodId, TraderXTransactionType transType, string msg)
    {
        TraderXTransactionResult result = new TraderXTransactionResult(transId, prodId, transType, ETraderXTransactionResultType.FAILURE, msg);
        result.needsSpaceConfirm = true;
        return result;
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
    bool NeedsSpaceConfirm() { return needsSpaceConfirm; }

    string ToStringFormatted()
    {
        return "TransactionId: " + transactionId + " ProductId: " + productId + " TransactionType: " + transactionType + " ResultType: " + resultType + " Message: " + message + "\n";
    }
}