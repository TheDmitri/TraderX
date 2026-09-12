class CfgPatches
{
	class TraderXCarLock_Script
	{
		units[]={};
		weapons[]={};
		requiredVersion=0.1;
		requiredAddons[]={"DZ_Data"};
	};
};
class CfgMods
{
	class TraderXCarLock
	{
		dir="TraderXCarLock";
		picture="";
		action="";
		hideName=1;
		hidePicture=1;
		name="TraderXCarLock";
		credits="";
		author="";
		authorID="0";
		version="1.0";
		extra=0;
		type="mod";
		dependencies[]=
		{
			"Game",
			"World"
		};
		class defs
		{
			// CarLock's own icon atlas (keyboard/close/delete/approved) — independent of traderx_icons.
			class imageSets
			{
				files[]=
				{
					"TraderXCarLock/datasets/icons/traderxcarlock_icons.imageset"
				};
			};
			// Do NOT list DayZular/CF script dirs here. Co-loaded core TraderX already
			// lists them into the shared script module, so DZlrComponentBase / CF_ModuleWorld
			// resolve anyway. Re-listing double-compiles DayZular and breaks 3_Game with
			// "Bad type 'JsonFileLoader'". TraderXCarLock hard-depends on core TraderX.
			class gameScriptModule
			{
				value="";
				files[]=
				{
					"TraderX/scripts/3_Game",
					"TraderXCarLock/scripts/3_Game"
				};
			};
			class worldScriptModule
			{
				value="";
				files[]=
				{
					"TraderX/scripts/4_World",
					"TraderXCarLock/scripts/4_World"
				};
			};
		};
	};
};
class CfgSoundShaders
{
	class TXCL_CarAlarm_SoundShader
	{
		samples[]=
		{
			{
				"TraderXCarLock\datasets\sounds\carAlarm",
				1
			}
		};
		radius = 500;
		range = 500;
		volume = 1;
		rangeCurve[]={
			{0,1},
			{50,1},
			{500,1}
		};
	};
	class TXCL_CarLock_SoundShader
	{
		samples[]=
		{
			{
				"TraderXCarLock\datasets\sounds\carLock",
				1
			}
		};
		range = 15;
		volume = 1;
	};
	class TXCL_CarUnlock_SoundShader
	{
		samples[]=
		{
			{
				"TraderXCarLock\datasets\sounds\carUnlock",
				1
			}
		};
		range = 15;
		volume = 1;
	};
};
class CfgSoundSets
{
	class TXCL_CarAlarm_SoundSet
	{
		soundShaders[]=
		{
			"TXCL_CarAlarm_SoundShader"
		};
	};
	class TXCL_CarLock_SoundSet
	{
		soundShaders[]=
		{
			"TXCL_CarLock_SoundShader"
		};
	};
	class TXCL_CarUnlock_SoundSet
	{
		soundShaders[]=
		{
			"TXCL_CarUnlock_SoundShader"
		};
	};
};
