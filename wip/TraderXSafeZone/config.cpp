class CfgPatches
{
	class TraderXSafeZone_Script
	{
		units[]={};
		weapons[]={};
		requiredVersion=0.1;
		requiredAddons[]={};
	};
};

class CfgMods
{
	class TraderXSafeZone
	{
		dir="TraderXSafeZone";
		picture="";
		action="";
		hideName=1;
		hidePicture=1;
		name="TraderXSafeZone";
		credits="O&N";
		author="TheDmitri";
		authorID="0";
		version="1.1";
		extra=0;
		type="mod";
		dependencies[]=
		{
			"Game",
			"World"
		};
		class defs
		{
			class gameScriptModule
			{
				value="";
				files[]=
				{
					"TraderXSafeZone/scripts/Common",
					"TraderXSafeZone/scripts/3_Game"
				};
			};
			class worldScriptModule
			{
				value="";
				files[]=
				{
					"TraderXSafeZone/scripts/Common",
					"TraderXSafeZone/scripts/4_World"
				};
			};
		};
	};
};
