class TraderXNpcService 
{
    static ref TraderXNpcService  m_instance;

    private ref map<int, ref array<PlayerBase>> playersPerNpc;

    private ref map<int, ref TraderXNpc> npcs;

    void TraderXNpcService ()
    {
       playersPerNpc = new map<int, ref array<PlayerBase>>();
       npcs = new map<int, ref TraderXNpc>();
    }

    static TraderXNpcService  GetInstance()
    {
        if (!m_instance)
        {
            m_instance = new TraderXNpcService ();
        }
        return m_instance;
    }

    TraderXNpc GetNpcById(int id)
    {
        return npcs.Get(id);
    }

    void CreateNpcs()
    {
        GetTraderXLogger().LogDebug("[TraderX] CreateNpcs initiated");
        foreach(TraderXNpc npc: GetTraderXModule().GetSettings().traders)
        {
            npcs.Insert(npc.npcId, npc);
            Object obj = GetGame().CreateObject(npc.className, vector.Zero, false, false);
            if (!obj)
            {
                GetTraderXLogger().LogDebug("[TraderX] obj was not created: "+ npc.className + " please make sure the syntaxe is correct!");
                continue;
            }

            PlayerBase traderPlayer = PlayerBase.Cast(obj);
            if (traderPlayer)
            {
                GetTraderXLogger().LogDebug("[TraderX] traderPlayer created and added!: ");
                traderPlayer.SetupTraderXNpc(npc);
                traderPlayer.SetTraderNpc();
                continue;
            }

            BuildingBase traderBuilding = BuildingBase.Cast(obj);
            if (traderBuilding)
            {
                GetTraderXLogger().LogDebug("[TraderX] traderStatic created and added!: ");
                traderBuilding.SetupTraderXNpc(npc);
                traderBuilding.SetTraderNpc();
            }
            else
            {
                GetTraderXLogger().LogDebug("[TraderX] traderStatic was NOT created ! Make sure your static object extends BuildingBase as the documentation tells you!");
            }
        }
    }
}
