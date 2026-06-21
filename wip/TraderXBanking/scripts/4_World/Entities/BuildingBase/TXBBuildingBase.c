// TraderXBanking - BuildingBase Extension
// Adds NPC ID support to buildings when TraderX is not loaded

#ifndef TraderX
modded class BuildingBase
{
	int netSync_buildingId = 0;

	void BuildingBase()
	{
		RegisterNetSyncVariableInt("netSync_buildingId");
	}

	void SetTraderXBuildingId(int buildingId)
	{
		netSync_buildingId = buildingId;
		SetSynchDirty();
	}

	int GetTraderXBuildingId()
	{
		return netSync_buildingId;
	}

	void SetNpcId(int npcId)
	{
		int id = (0x0000007F & npcId);
		SetTraderXBuildingId((netSync_buildingId & 0xFFFFFF80) | id);
	}

	int GetNpcId()
	{
		return netSync_buildingId & 0x0000007F;
	}

	bool IsTraderNpc()
	{
		return (netSync_buildingId & 0x00000080) == 0x80;
	}

	void SetTraderNpc()
	{
		SetTraderXBuildingId(netSync_buildingId | 0x80);
	}

	// Bank NPC setup for buildings (simplified)
	void SetupTXBBankNpc(vector position, vector orientation)
	{
		SetNpcId(TXBConstants.BANK_ID);
		SetPosition(position);
		SetOrientation(orientation);
		SetAllowDamage(false);
	}
}
#endif // !TraderX
