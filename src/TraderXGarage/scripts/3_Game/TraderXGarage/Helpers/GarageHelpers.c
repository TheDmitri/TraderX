// TraderXGarage - GarageHelpers
// Pure data/string utility methods. No 4_World dependencies (no PlayerBase, CarScript, EntityAI).

class GarageHelpers
{
	static int GetLowSteamId(string steamId64)
	{
		int id = steamId64.ToInt();
		return id;
	}

	static string ConvertHealthToLevel(string className, float health)
	{
		if (health >= 80)
			return "PRISTINE";
		else if (health >= 60)
			return "WORN";
		else if (health >= 40)
			return "DAMAGED";
		else if (health >= 20)
			return "BADLY DAMAGED";

		return "RUINED";
	}
}
