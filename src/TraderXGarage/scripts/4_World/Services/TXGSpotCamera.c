// TraderXGarage - Spot camera (client)
// Scripted camera orbiting the parking spot while the hologram of the selected vehicle stands there.

class TXGSpotCamera
{
    protected Camera m_Camera;
    protected vector m_Target;
    protected float  m_Yaw;
    protected float  m_Distance = 9.0;

    static const float MIN_DISTANCE = 4.0;
    static const float MAX_DISTANCE = 18.0;

    protected static Camera s_Camera;
    protected bool m_Active;

    bool Start(vector spotPos, vector spotOri)
    {
        if (!s_Camera)
            s_Camera = Camera.Cast(GetGame().CreateObject("staticcamera", spotPos + Vector(0, 2, 0), true));
        if (!s_Camera)
            return false;
        m_Camera = s_Camera;
        m_Target = spotPos + Vector(0, 1.0, 0);
        m_Yaw = spotOri[0] + 140;   // front three-quarter view
        Apply();
        m_Camera.SetActive(true);
        m_Active = true;
        return true;
    }

    bool IsActive()
    {
        return m_Active;
    }

    void Rotate(float degrees)
    {
        m_Yaw = m_Yaw + degrees;
        Apply();
    }

    void Zoom(float metres)
    {
        m_Distance = Math.Clamp(m_Distance + metres, MIN_DISTANCE, MAX_DISTANCE);
        Apply();
    }

    // Same exit as BaseCameras' laptop: camera off, delete it, then SelectPlayer(NULL, player) gives
    // the view back to the body. Queued to the next frame, outside the widget click handler.
    void Stop()
    {
        if (!m_Active)
            return;
        m_Active = false;
        m_Camera = null;
        GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(TXGSpotCamera.ReturnToPlayer, 0, false);
    }

    static void ReturnToPlayer()
    {
        if (s_Camera)
        {
            s_Camera.SetActive(false);
            GetGame().ObjectDelete(s_Camera);
            s_Camera = null;
        }
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (player)
            GetGame().SelectPlayer(null, player);
        if (GetTXGUIService())
            GetTXGUIService().ReapplyInputLock();
    }

    protected void Apply()
    {
        if (!m_Camera)
            return;
        float rad = m_Yaw * Math.DEG2RAD;
        vector pos = m_Target + Vector(Math.Sin(rad) * m_Distance, 1.5 + m_Distance * 0.2, Math.Cos(rad) * m_Distance);
        float ground = GetGame().SurfaceY(pos[0], pos[2]) + 0.5;
        if (pos[1] < ground)
            pos[1] = ground;
        m_Camera.SetPosition(pos);
        vector dir = m_Target - pos;
        m_Camera.SetOrientation(dir.VectorToAngles());
    }
}
