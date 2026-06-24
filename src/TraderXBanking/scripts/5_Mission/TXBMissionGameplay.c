// TraderXBanking - Client Mission Integration
// Handles ESC key to close banking menu

modded class MissionGameplay
{
	override void OnKeyRelease(int key)
	{
		super.OnKeyRelease(key);

		if (key == KeyCode.KC_ESCAPE)
		{
			if (GetTXBUIService() && GetTXBUIService().IsMenuOpen())
			{
				GetTXBUIService().CloseBankingMenu();
			}
		}
	}
}
