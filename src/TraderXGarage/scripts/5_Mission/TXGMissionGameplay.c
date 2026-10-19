// TraderXGarage - Client Mission Integration
// ESC leaves the camera view, then closes the garage. A/D/W/S (and arrows) drive the camera view.

modded class MissionGameplay
{
    override void OnKeyPress(int key)
    {
        super.OnKeyPress(key);
        if (GetTXGUIService())
            GetTXGUIService().HandleKey(key);
    }

    override void OnKeyRelease(int key)
    {
        super.OnKeyRelease(key);

        if (key == KeyCode.KC_ESCAPE)
        {
            if (GetTXGUIService() && GetTXGUIService().IsMenuOpen())
                GetTXGUIService().OnEscape();
        }
    }
}
