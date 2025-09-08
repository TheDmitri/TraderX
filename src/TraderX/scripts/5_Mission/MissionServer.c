modded class MissionServer extends MissionBase
{
	override void OnPlayerJoined(PlayerBase player, PlayerIdentity identity)
	{
		super.OnPlayerJoined(player, identity);
		GetTraderXModule().OnPlayerJoined(player, identity);
	}
};
