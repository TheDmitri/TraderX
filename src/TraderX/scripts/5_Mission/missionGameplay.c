modded class MissionGameplay extends MissionBase
{
	override void Pause()
	{
		if(GetTraderXModule().IsTraderViewOpen())
			return;

		super.Pause();
	}

	override void OnUpdate( float timeslice )
	{
		super.OnUpdate(timeslice);
		if (GetUApi() && GetUApi().GetInputByName("UAUIBack").LocalPress()){
            GetTraderXModule().CloseTraderView();
        }
	}
};
