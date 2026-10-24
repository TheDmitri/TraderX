class CfgPatches
{
	class TraderXBanking_Script
	{
		units[]={};
		weapons[]={};
		requiredVersion=0.1;
		requiredAddons[] = {"DZ_Data","TraderX_DayZular"};
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
			class widgetStyles
			{
				files[]=
				{
					"TraderXBanking/datasets/styles/banking_styles.styles"
				};
			};
			class imageSets
			{
				files[]=
				{
					"TraderXBanking/datasets/styles/banking_ui.imageset",
				};
			};
			class gameScriptModule
			{
				value="";
				files[]=
				{
					"TraderXDayZular/scripts/3_Game",
					"TraderX/scripts/3_Game",
					"TraderXBanking/scripts/Common",
					"TraderXBanking/scripts/3_Game"
				};
			};
			class worldScriptModule
			{
				value="";
				files[]=
				{
					"TraderXDayZular/scripts/4_World",
					"TraderX/scripts/4_World",
					"TraderXBanking/scripts/Common",
					"TraderXBanking/scripts/4_World"
				};
			};
			class missionScriptModule
			{
				value="";
				files[]=
				{
					"TraderXDayZular/scripts/5_Mission",
					"TraderX/scripts/5_Mission",
					"TraderXBanking/scripts/Common",
					"TraderXBanking/scripts/5_Mission"
				};
			};
		};
	};
};
class CfgSoundShaders
{
	class TXB_Sound_Select_Event_SoundShader
	{
		samples[]=
		{

			{
				"TraderXBanking\datasets\sounds\select_sound",
				1
			}
		};
		volume=1.4;
		range=5;
	};
};
class CfgSoundSets
{
	class TXB_Sound_Select_Event_SoundSet
	{
		soundShaders[]=
		{
			"TXB_Sound_Select_Event_SoundShader"
		};
		volumeFactor=1.4;
		spatial=1;
		doppler=1;
		loop=0;
	};
};

class CfgVehicles
{
	class Paper;
	// Bank card required when requireCreditCard is on (model shipped in TraderPlusMoney)
	class TXBCreditCard: Paper
	{
		scope = 2;
		displayName = "$STR_txb_credit_card";
		descriptionShort = "$STR_txb_credit_card_des";
		itemSize[] = {1,1};
		itemsCargoSize[] = {0,0};
		model = "TraderPlusMoney\Data\money\Debit_Card.p3d";
	};
	// TraderPlus V1 card: players keep it and it still opens the bank
	class TraderPlusCreditCard: TXBCreditCard
	{
		scope = 2;
	};
};
