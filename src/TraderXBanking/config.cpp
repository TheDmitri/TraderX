class CfgPatches
{
	class TraderXBanking_Script
	{
		units[]={};
		weapons[]={};
		requiredVersion=0.1;
		requiredAddons[]={"DZ_Data"};
	};
};
class CfgMods
{
	class TraderXBanking
	{
		dir="TraderXBanking";
		picture="";
		action="";
		hideName=1;
		hidePicture=1;
		name="TraderXBanking";
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
					"TraderXBanking/scripts/Common",
					"TraderXBanking/scripts/3_Game"
				};
			};
			class worldScriptModule
			{
				value="";
				files[]=
				{
					"DayZular/scripts/4_World",
					"TraderXBanking/scripts/Common",
					"TraderXBanking/scripts/4_World"
				};
			};
			class missionScriptModule
			{
				value="";
				files[]=
				{
					"DayZular/scripts/5_Mission",
					"TraderXBanking/scripts/Common",
					"TraderXBanking/scripts/5_Mission"
				};
			};
		};
	};
};
