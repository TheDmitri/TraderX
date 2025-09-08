
/*
   Information regarding netSync_playerId:

   This integer contains multiiple variable in order to reduce the number of sync variable accross the network 
   and also because we don't always need 32 bits for our use case.

   the first 0 to 7 bit are reserved for the id value which leaves the ability to have a range of value between 0 and 127.

   the 8 bit is reserved for the identification of a trader npc
*/
modded class PlayerBase extends ManBase
{
	int	netSync_playerId = 0;

	override void Init()
	{
		super.Init();
		RegisterNetSyncVariableInt("netSync_playerId");
	}

	void SetTraderXPlayerId(int playerId)
	{
		netSync_playerId = playerId;
		SetSynchDirty();
	}

	int GetTraderXPlayerId()
	{
		return netSync_playerId;
	}

	void SetNpcId(int npcId)
	{
		int id = (0x0000007F & npcId);
		SetTraderXPlayerId((netSync_playerId & 0xFFFFFF80) | id);
	}

	int GetNpcId()
	{
		return netSync_playerId & 0x0000007F;
	}

	//Npc Logic
	void SetupTraderXNpc(TraderXNpc tpNpc)
	{
		SetNpcId(tpNpc.npcId);
		SetPosition(tpNpc.position);
		SetOrientation(tpNpc.orientation);
		SetAllowDamage(false);

		TraderXLoadoutService.GetInstance().EquipPlayer(this, tpNpc.loadouts);
	}

	bool IsTraderNpc()
	{
		return (netSync_playerId & 0x00000080) == 0x80;
	}

	void SetTraderNpc()
	{
		SetTraderXPlayerId(netSync_playerId | 0x80);	

		string output;
		TraderXCoreUtils.PrintIntegerBits(netSync_playerId, output);
		//GetTraderXLogger().LogDebug("netSync_playerId :");
		//GetTraderXLogger().LogDebug(output);
	}

	override void SetActions()
	{
		super.SetActions();
		AddAction(ActionTraderXMenu);
	}
};