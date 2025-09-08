////////////////////////////////////////////////////////////////////
//DeRap: TraderX\config.bin
//Produced from mikero's Dos Tools Dll version 8.57
//https://mikero.bytex.digital/Downloads
//'now' is Mon May 29 15:45:34 2023 : 'file' last modified on Thu Jan 01 01:00:01 1970
////////////////////////////////////////////////////////////////////

#define _ARMA_

class CfgPatches
{
	class TraderX_Script
	{
		units[] = {};
		weapons[] = {};
		requiredVersion = 0.1;
		requiredAddons[] = {"DZ_Data"};
	};
};
class CfgMods
{
	class TraderX
	{
		dir = "TraderX";
		picture = "";
		action = "";
		hideName = 1;
		hidePicture = 1;
		name = "TraderX";
		credits = "TheDmitri,Chubby,Morty";
		author = "TheDmitri";
		authorID = "0";
		version = "1.1";
		extra = 0;
		type = "mod";
		dependencies[] = {"Game","World","Mission"};
		class defs
		{
			class widgetStyles
			{
				files[]=
				{
					"TraderX/datasets/Styles.styles"
				};
			};
			class imageSets
			{
				files[]=
				{
					"TraderX/datasets/images/notification_ui.imageset",
					"TraderX/datasets/traderplus_ui.imageset",
					"TraderX/datasets/icons/icons.imageset",
				};
			};
			class gameScriptModule
			{
				value = "";
				files[] = {"DabsFramework/scripts/3_Game","TraderPlus/scripts/Common","TraderX/scripts/Common","TraderX/scripts/3_Game"};
			};
			class worldScriptModule
			{
				value = "";
				files[] = {"DabsFramework/scripts/4_World","TraderPlus/scripts/Common","TraderX/scripts/Common","TraderX/scripts/4_World"};
			};
			class missionScriptModule
			{
				value = "";
				files[] = {"DabsFramework/scripts/5_Mission","TraderPlus/scripts/Common","TraderX/scripts/Common","TraderX/scripts/5_Mission"};
			};
		};
	};
};
class CfgSoundShaders
{
	class TraderX_Sound_Coins_SoundShader
	{
		samples[]=
		{
			{
				"TraderX\datasets\sounds\coins",
				1
			}
		};
		volume=1;
		range=5;
	};
	class TraderX_Sound_Error_SoundShader
	{
		samples[]=
		{

			{
				"TraderX\datasets\sounds\error",
				1
			}
		};
		volume=1;
		range=5;
	};
	class TraderX_Sound_Quick_Event_SoundShader
	{
		samples[]=
		{

			{
				"TraderX\datasets\sounds\quick_event",
				1
			}
		};
		volume=1.4;
		range=5;
	};
	class TraderX_Sound_Confirm_SoundShader
	{
		samples[]=
		{

			{
				"TraderX\datasets\sounds\confirm",
				1
			}
		};
		volume=1.4;
		range=5;
	};
};
class CfgSoundSets
{
	class TraderX_Sound_Coins_SoundSet
	{
		soundShaders[]=
		{
			"TraderX_Sound_Coins_SoundShader"
		};
		volumeFactor=1.4;
		spatial=1;
		doppler=1;
		loop=0;
	};
	class TraderX_Sound_Error_SoundSet
	{
		soundShaders[]=
		{
			"TraderX_Sound_Error_SoundShader"
		};
		volumeFactor=1.4;
		spatial=1;
		doppler=1;
		loop=0;
	};
	class TraderX_Sound_Quick_Event_SoundSet
	{
		soundShaders[]=
		{
			"TraderX_Sound_Quick_Event_SoundShader"
		};
		volumeFactor=1.4;
		spatial=1;
		doppler=1;
		loop=0;
	};
	class TraderX_Sound_Confirm_SoundSet
	{
		soundShaders[]=
		{
			"TraderX_Sound_Confirm_SoundShader"
		};
		volumeFactor=1.4;
		spatial=1;
		loop=0;
	};
};

