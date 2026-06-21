modded class MissionGameplay extends MissionBase
{
	override void Pause()
	{
		if(GetTraderXUIService() && GetTraderXUIService().IsTraderViewOpen())
			return;

		super.Pause();
	}

	override void OnUpdate( float timeslice )
	{
		super.OnUpdate(timeslice);
		if (GetUApi() && GetUApi().GetInputByName("UAUIBack").LocalPress()){
            if(GetTraderXUIService()) GetTraderXUIService().CloseTraderView();
        }
	}
};
