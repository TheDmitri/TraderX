// TraderXGarage - PlayerBase Extension
// Modded PlayerBase to support garage NPC identification and action registration.

modded class PlayerBase
{
	int m_TXGarageNpcId = 0;

	void SetupTXGarageNpc(GarageNpc npcConfig)
	{
		m_TXGarageNpcId = TXG_GARAGE_NPC_ID;
	}

	override void SetActions()
	{
		super.SetActions();
		AddAction(ActionGarageMenu);
	}
}
