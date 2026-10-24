modded class BoatScript
{
    private int m_boatUniqueId = 0;
    private static ref map<int, BoatScript> m_MapAllBoats = new map<int, BoatScript>;

    string m_TraderX_LastDriverID = "";

    // Saved by this build (TraderX ModStorage block present): the raw boat id follows CF's block
    protected bool m_TraderX_HasStorageBlock;

    void BoatScript()
	  {
      RegisterNetSyncVariableInt("m_boatUniqueId");

      if(GetGame().IsClient())
        return;
        
      HandleCarDamage();
	  }

    void HandleCarDamage()
    {
      SetAllowDamage(false);

      if(IsServerAllowDamage())
      {
        GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DisableGodMode, 30000);
      }
    }

    bool IsServerAllowDamage()
    {
      return true;
    }

    bool IsLastDriver(PlayerBase player)
    {
      return m_TraderX_LastDriverID == player.GetIdentity().GetPlainId();
    }

    void ~BoatScript()
    {
      if(m_MapAllBoats)
   		m_MapAllBoats.Remove(m_boatUniqueId);
    }

    static map<int, BoatScript> GetMapAll()
   	{
   		return m_MapAllBoats;
   	}

    int GetCarUniqueId()
    {
      return m_boatUniqueId;
    }

    void SetCarUniqueId(int id)
    {
      m_boatUniqueId = id;
      m_MapAllBoats.Set(id, this);
    }

    override void OnEngineStart()
 	  {
        super.OnEngineStart();
        PlayerBase player = PlayerBase.Cast(CrewMember(DayZPlayerConstants.VEHICLESEAT_DRIVER));
        if (player)
          m_TraderX_LastDriverID = player.GetIdentity().GetPlainId();
 	  }

    void DisableGodMode()
    {
      SetAllowDamage(true);
    }

    override void OnStoreSave( ParamsWriteContext ctx )
     {
        super.OnStoreSave(ctx);
        Param1<int> data = new Param1<int>(m_boatUniqueId);
        ctx.Write(data);
     }

    override void CF_OnStoreSave(CF_ModStorageMap storage)
    {
        super.CF_OnStoreSave(storage);

        CF_ModStorage ctx = storage.Get(TRADERX_STORAGE_MOD);
        if (ctx)
            ctx.Write(m_boatUniqueId);
    }

    override bool CF_OnStoreLoad(CF_ModStorageMap storage)
    {
        if (!super.CF_OnStoreLoad(storage))
            return false;

        m_TraderX_HasStorageBlock = (storage.Get(TRADERX_STORAGE_MOD) != null);
        return true;
    }

    override bool OnStoreLoad( ParamsReadContext ctx, int version )
      {
        if ( !super.OnStoreLoad( ctx, version ) )
          return false;

            // Never saved by this build: only an earlier TraderX wrote a boat id (TraderPlus wrote nothing)
            if (!m_TraderX_HasStorageBlock && TraderXWorldFormat.Get() != TraderXWorldFormat.TRADERX)
              return true;

            Param1<int>data = new Param1<int>(0);
            if (ctx.Read(data))
            {
              m_boatUniqueId = data.param1;
              SetCarUniqueId(m_boatUniqueId);
            }
            return true;
      }
}
