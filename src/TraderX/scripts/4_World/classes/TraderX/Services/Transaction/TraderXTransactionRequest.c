class TraderXTransactionRequest
{
    private string m_SteamId;
    private PlayerBase m_Player;
    private ref TraderXTransactionCollection m_TransactionCollection;
    private int m_NpcId;
    
    void TraderXTransactionRequest(string steamId, PlayerBase player, TraderXTransactionCollection transactionCollection, int npcId)
    {
        m_SteamId = steamId;
        m_Player = player;
        m_TransactionCollection = transactionCollection;
        m_NpcId = npcId;
    }
    
    static TraderXTransactionRequest Create(string steamId, PlayerBase player, TraderXTransactionCollection transactionCollection, int npcId)
    {
        return new TraderXTransactionRequest(steamId, player, transactionCollection, npcId);
    }
    
    string GetSteamId()
    {
        return m_SteamId;
    }
    
    PlayerBase GetPlayer()
    {
        return m_Player;
    }
    
    TraderXTransactionCollection GetTransactionCollection()
    {
        return m_TransactionCollection;
    }
    
    int GetNpcId()
    {
        return m_NpcId;
    }
    
    bool IsValid()
    {
        return m_SteamId != "" && m_Player && m_TransactionCollection && !m_TransactionCollection.IsEmpty();
    }
    
    // Méthodes de délégation pour faciliter l'utilisation
    array<ref TraderXTransaction> GetAllTransactions()
    {
        return m_TransactionCollection.GetAllTransactions();
    }
    
    int GetTransactionCount()
    {
        return m_TransactionCollection.GetCount();
    }
    
    void SortTransactionsByDepth()
    {
        m_TransactionCollection.SortByDepth();
    }
    
    TraderXTransactionPrice GetTotalPrice()
    {
        return m_TransactionCollection.GetTotalPrice();
    }
}
