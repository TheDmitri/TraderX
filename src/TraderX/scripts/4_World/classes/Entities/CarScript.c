modded class CarScript
{
    private int m_traderX_carUniqueId = 0;
    private static ref map<int, CarScript> m_traderX_MapAllCars = new map<int, CarScript>;

    string m_TraderX_LastDriverID = "";

    void CarScript()
	  {
      RegisterNetSyncVariableInt("m_traderX_carUniqueId");

      if(GetGame().IsClient())
        return;
        
      HandleTXCarDamage();
	  }

    void HandleTXCarDamage()
    {
      SetAllowDamage(false);

      if(IsTXServerAllowDamage())
      {
        GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DisableTXGodMode, 30000);
      }
    }

    bool IsTXServerAllowDamage()
    {
      return true;
    }

    bool IsLastDriver(PlayerBase player)
    {
      return m_TraderX_LastDriverID == player.GetIdentity().GetPlainId();
    }

    void ~CarScript()
    {
      if(m_traderX_MapAllCars)
   		m_traderX_MapAllCars.Remove(m_traderX_carUniqueId);
    }

    static map<int, CarScript> GetTXMapAll()
   	{
   		return m_traderX_MapAllCars;
   	}

    int GetTXCarUniqueId()
    {
      return m_traderX_carUniqueId;
    }

    void SetTXCarUniqueId(int id)
    {
      m_traderX_carUniqueId = id;
      m_traderX_MapAllCars.Set(id, this);
    }

    override void OnEngineStart()
 	  {
        super.OnEngineStart();
        PlayerBase player = PlayerBase.Cast(CrewMember(DayZPlayerConstants.VEHICLESEAT_DRIVER));
        if (player)
          m_TraderX_LastDriverID = player.GetIdentity().GetPlainId();
 	  }

    void DisableTXGodMode()
    {
      SetAllowDamage(true);
    }

    override void OnStoreSave( ParamsWriteContext ctx )
     {
        super.OnStoreSave(ctx);
        Param1<int> data = new Param1<int>(m_traderX_carUniqueId);
        ctx.Write(data);
     }

    override bool OnStoreLoad( ParamsReadContext ctx, int version )
      {
        if ( !super.OnStoreLoad( ctx, version ) )
          return false;

            Param1<int>data = new Param1<int>(0);
            if (ctx.Read(data))
            {
              m_traderX_carUniqueId = data.param1;
              SetTXCarUniqueId(m_traderX_carUniqueId);
            }
            return true;
      }
}
