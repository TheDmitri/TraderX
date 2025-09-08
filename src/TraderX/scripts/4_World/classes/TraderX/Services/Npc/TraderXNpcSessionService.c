class TraderXNpcSessionService 
{
    static ref TraderXNpcSessionService  m_instance;

    private ref map<int, ref array<PlayerBase>> playersPerNpc;

    void TraderXNpcSessionService ()
    {
       playersPerNpc = new map<int, ref array<PlayerBase>>();
    }

    static TraderXNpcSessionService  GetInstance()
    {
        if (!m_instance)
        {
            m_instance = new TraderXNpcSessionService ();
        }
        return m_instance;
    }

    void AddPlayerToNpcId(PlayerBase player, int npcId)
    {
        if (!playersPerNpc.Contains(npcId)){
            playersPerNpc.Insert(npcId, new array<PlayerBase>());
        }
        
        playersPerNpc[npcId].Insert(player);
    }

    void RemovePlayerFromNpcId(PlayerBase player, int npcId)
    {
        if (!playersPerNpc.Contains(npcId))
            return;
        
        array<PlayerBase> players = playersPerNpc[npcId];

        for(int i = players.Count() - 1; i >= 0; i--)
        {
            if(players[i] == player)
            {
                players.Remove(i);
                break;
            }
        }
    }

    void RefreshPlayers(int npcId)
    {
        if (!playersPerNpc.Contains(npcId))
            return;

        array<PlayerBase> players = new array<PlayerBase>();
        array<PlayerBase> playersNpc = playersPerNpc[npcId];

        foreach(PlayerBase player: playersNpc)
        {
            if(!player)
                continue;
            
            players.Insert(player);
        }

        playersPerNpc[npcId].Clear();
        playersPerNpc.Set(npcId, players);
    }

    void SendStockResponses(array<ref TraderXProductStock> itemsStock, int npcId)
    {
        if (!playersPerNpc.Contains(npcId))
            return;
        
        array<PlayerBase> players = playersPerNpc[npcId];
        if(!players){
            GetTraderXLogger().LogError("SendStockResponses: No players found for npcId: " + npcId);
            return;
        }

        foreach(PlayerBase player: players)
        {
            if(!player)
                continue;

            GetRPCManager().SendRPC("TraderX", "GetTraderStockResponse", new Param1<array<ref TraderXProductStock>>(itemsStock), true, player.GetIdentity());
        }

        RefreshPlayers(npcId);
    }
}
