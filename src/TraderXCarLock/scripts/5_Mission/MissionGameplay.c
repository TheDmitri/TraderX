modded class MissionGameplay extends MissionBase
{
    const int CARLOCK_DISTANCE_RADIUS = 8;

    override void OnUpdate(float timeslice)
    {
        super.OnUpdate(timeslice);

        if (!GetUIManager().GetMenu() && GetUApi() && GetUApi().GetInputByName("UACarLockToggle").LocalPress())
        {
            GetTXCarLockLogger().LogDebug("TraderXCarLock::MissionGameplay::OnUpdate::UACarLockToggle");
            TryToLockCarNearby();
        }

        if (GetUApi() && GetUApi().GetInputByName("UAUIBack").LocalPress())
        {
            if (GetTXCarLockUIService() && GetTXCarLockUIService().IsMenuOpen())
            {
                GetTXCarLockUIService().CloseCarLockMenu();
            }
        }
    }

    void TryToLockCarNearby()
    {
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());

        array<Object> objs = new array<Object>();

        GetGame().GetObjectsAtPosition3D(player.GetPosition(), CARLOCK_DISTANCE_RADIUS, objs, null);
        if (objs.Count() < 1)
            return;

        foreach (Object obj : objs)
        {
            VehicleLockManager manager = VehicleLockManager.Get(obj);
            if (manager && manager.HasVehicleLock() && player.CanInteractWithVehicleLock(manager))
            {
                GetDayZularRPCManager().SendRPC("TXCarLock", "GetCarLockLockingRequest", new Param3<Transport, int, bool>(manager.GetTransport(), manager.GetVehicleLockPassword(), !manager.IsVehicleLocked()), true);
            }
        }
    }
}
