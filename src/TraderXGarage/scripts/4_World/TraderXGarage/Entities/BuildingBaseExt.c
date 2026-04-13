// TraderXGarage - BuildingBase Extension
// Modded BuildingBase to support garage NPC identification.

modded class BuildingBase
{
	int m_TXGarageNpcId = 0;

	void SetupTXGarageNpc(GarageNpc npcConfig)
	{
		m_TXGarageNpcId = TXG_GARAGE_NPC_ID;
	}
}
