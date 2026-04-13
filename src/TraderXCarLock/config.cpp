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
		credits="TheDmitri";
		author="TheDmitri";
		authorID="0";
		version="1.0";
		extra=0;
		type="mod";
		inputs = "TraderXCarLock/inputs.xml";
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
					"TraderXCarLock/scripts/Common",
					"TraderXCarLock/scripts/3_Game"
				};
			};
			class worldScriptModule
			{
				value="";
				files[]=
				{
					"TraderXCarLock/scripts/Common",
					"TraderXCarLock/scripts/4_World"
				};
			};
			class missionScriptModule
			{
				value="";
				files[]=
				{
					"TraderXCarLock/scripts/Common",
					"TraderXCarLock/scripts/5_Mission"
				};
			};
		};
	};
};
class CfgSoundShaders
{
	class TXcl_CarAlarm_SoundShader
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
	class TXcl_CarLock_SoundShader
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
	class TXcl_CarUnlock_SoundShader
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
	class TXcl_CarAlarm_SoundSet
	{
		soundShaders[]=
		{
			"TXcl_CarAlarm_SoundShader"
		};
	};
	class TXcl_CarLock_SoundSet
	{
		soundShaders[]=
		{
			"TXcl_CarLock_SoundShader"
		};
	};
	class TXcl_CarUnlock_SoundSet
	{
		soundShaders[]=
		{
			"TXcl_CarUnlock_SoundShader"
		};
	};
};
