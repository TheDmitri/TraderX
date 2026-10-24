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
		// TAB / E next page, Q previous page while the trader is open
		if (GetUApi() && GetTraderXUIService() && GetTraderXUIService().IsTraderViewOpen() && TraderXAppComponent.GetInstance())
		{
			if (GetUApi().GetInputByName("UAGear").LocalPress() || GetUApi().GetInputByName("UAUITabRight").LocalPress())
				TraderXAppComponent.GetInstance().CyclePage(1);
			else if (GetUApi().GetInputByName("UAUITabLeft").LocalPress())
				TraderXAppComponent.GetInstance().CyclePage(-1);
		}
	}
};
