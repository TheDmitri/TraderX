// TraderXGarage - Client Mission Integration
// ESC closes the garage menu.

modded class MissionGameplay
{
    override void OnKeyRelease(int key)
    {
        super.OnKeyRelease(key);

        if (key == KeyCode.KC_ESCAPE)
        {
            if (GetTXGUIService() && GetTXGUIService().IsMenuOpen())
            {
                GetTXGUIService().CloseGarageMenu();
            }
        }
    }
}
