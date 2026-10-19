// TraderXGarage - Server Mission Integration
// Flags the shutdown before the engine deletes every entity (see TXGVehicleStampHelper).

modded class MissionServer
{
    override void OnMissionFinish()
    {
        TXGVehicleStampHelper.s_ServerShuttingDown = true;
        super.OnMissionFinish();
    }
}
