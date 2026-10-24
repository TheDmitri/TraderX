modded class CarScript
{
    private int m_traderX_carUniqueId = 0;
    private static ref map<int, CarScript> m_traderX_MapAllCars = new map<int, CarScript>;

    string m_TraderX_LastDriverID = "";

    // Saved by this build (TraderX ModStorage block present): the raw vehicle id follows CF's block
    protected bool m_TraderX_HasStorageBlock;
    // TraderPlus V1 CarLock state found in a vehicle saved by TraderPlus (applied by TraderXCarLock)
    bool m_TraderX_TPLockFound;
    bool m_TraderX_TPLocked;
    int  m_TraderX_TPLockOwner = -1;
    int  m_TraderX_TPLockPassword = -1;

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

    override void CF_OnStoreSave(CF_ModStorageMap storage)
    {
        super.CF_OnStoreSave(storage);

        CF_ModStorage ctx = storage.Get(TRADERX_STORAGE_MOD);
        if (ctx)
            ctx.Write(m_traderX_carUniqueId);
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

            // Never saved by this build: read what the previous mod left (see TraderXWorldFormat)
            if (!m_TraderX_HasStorageBlock)
            {
              string format = TraderXWorldFormat.Get();
              if (format == TraderXWorldFormat.VANILLA)
                return true;

              // TraderPlus V1: its own vehicle id (same Param1 as ours), then TraderPlus CarLock's lock
              if (format == TraderXWorldFormat.TRADERPLUS)
              {
                Param1<int> tpId = new Param1<int>(0);
                if (ctx.Read(tpId))
                {
                  m_traderX_carUniqueId = tpId.param1;
                  SetTXCarUniqueId(m_traderX_carUniqueId);
                }

                Param3<bool, int, int> tpLock = new Param3<bool, int, int>(false, -1, -1);
                if (ctx.Read(tpLock))
                {
                  m_TraderX_TPLockFound = true;
                  m_TraderX_TPLocked = tpLock.param1;
                  m_TraderX_TPLockOwner = tpLock.param2;
                  m_TraderX_TPLockPassword = tpLock.param3;
                }
                return true;
              }
            }

            Param1<int>data = new Param1<int>(0);
            if (ctx.Read(data))
            {
              m_traderX_carUniqueId = data.param1;
              SetTXCarUniqueId(m_traderX_carUniqueId);
            }
            return true;
      }
}
