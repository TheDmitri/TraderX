class CfgPatches
{
	class TraderXGarage_Script
	{
		units[]={};
		weapons[]={};
		requiredVersion=0.1;
		requiredAddons[]={"DZ_Data"};
	};
};
class CfgMods
{
	class TraderXGarage
	{
		dir="TraderXGarage";
		picture="";
		action="";
		hideName=1;
		hidePicture=1;
		name="TraderXGarage";
		credits="";
		author="";
		authorID="0";
		version="1.0";
		extra=0;
		type="mod";
		dependencies[]=
		{
			"Game",
			"World",
			"Mission"
		};
		class defs
		{
			class gameScriptModule
			{
				value="";
				files[]=
				{
					"DayZular/scripts/3_Game",
					"TraderXGarage/scripts/Common",
					"TraderXGarage/scripts/3_Game"
				};
			};
			class worldScriptModule
			{
				value="";
				files[]=
				{
					"DayZular/scripts/4_World",
					"TraderXGarage/scripts/Common",
					"TraderXGarage/scripts/4_World"
				};
			};
			class missionScriptModule
			{
				value="";
				files[]=
				{
					"DayZular/scripts/5_Mission",
					"TraderXGarage/scripts/Common",
					"TraderXGarage/scripts/5_Mission"
				};
			};
		};
	};
};
