modded class BoatScript
{
    private int m_boatUniqueId = 0;
    private static ref map<int, BoatScript> m_MapAllBoats = new map<int, BoatScript>;

    string m_TraderX_LastDriverID = "";

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

    override bool OnStoreLoad( ParamsReadContext ctx, int version )
      {
        if ( !super.OnStoreLoad( ctx, version ) )
          return false;

            Param1<int>data = new Param1<int>(0);
            if (ctx.Read(data))
            {
              m_boatUniqueId = data.param1;
              SetCarUniqueId(m_boatUniqueId);
            }
            return true;
      }
}
