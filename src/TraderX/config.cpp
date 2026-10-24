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
		requiredAddons[] = {"DZ_Data","JM_CF_Scripts","TraderX_DayZular","money"};
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
		storageVersion = 1;
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
					"TraderX/datasets/Styles.styles",
					"TraderX/datasets/styles/traderx_styles.styles"
				};
			};
			class imageSets
			{
				files[]=
				{
					"TraderX/datasets/images/notification_ui.imageset",
					"TraderX/datasets/traderplus_ui.imageset",
					"TraderX/datasets/styles/traderx_ui.imageset",
					"TraderX/datasets/icons/traderx_icons.imageset"
				};
			};
			class gameScriptModule
			{
				value = "";
				files[] = {"TraderXDayZular/scripts/Common","TraderXDayZular/scripts/3_Game","TraderPlus/scripts/Common","TraderX/scripts/Common","TraderX/scripts/3_Game"};
			};
			class worldScriptModule
			{
				value = "";
				files[] = {"TraderXDayZular/scripts/Common","TraderXDayZular/scripts/4_World","TraderPlus/scripts/Common","TraderX/scripts/Common","TraderX/scripts/4_World"};
			};
			class missionScriptModule
			{
				value = "";
				files[] = {"TraderXDayZular/scripts/Common","TraderXDayZular/scripts/5_Mission","TraderPlus/scripts/Common","TraderX/scripts/Common","TraderX/scripts/5_Mission"};
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
	class TraderX_Sound_Select_SoundShader
	{
		samples[]=
		{

			{
				"TraderX\datasets\sounds\select_sound",
				1
			}
		};
		volume=1;
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
	class TraderX_Sound_Select_SoundSet
	{
		soundShaders[]=
		{
			"TraderX_Sound_Select_SoundShader"
		};
		volumeFactor=1;
		spatial=1;
		loop=0;
	};
};

class CfgVehicles
{
	class HouseNoDestruct;
	class Inventory_Base;
	class TraderPlusMoney_Base;
	class Wad_Base;
	// TraderPlus V1 money: same notes and coins as TraderX_Money_* under their TraderPlus names, so the
	// cash players hold on a TraderPlus server is kept (same base class names as TraderPlus)
	class TraderPlus_Wallet: Inventory_Base
	{
		scope = 2;
		displayName = "$STR_Wallet";
		descriptionShort = "$STR_Wallet_des";
		model = "TraderPlusMoney\Data\Wallet\Wallet.p3d";
		weight = 130;
		itemSize[] = {2,2};
		itemsCargoSize[] = {6,4};
		canBeDigged = 0;
		allowOwnedCargoManipulation = 1;
	};
	class TraderPlusCoin_Base: Inventory_Base
	{
		scope = 0;
		overrideDrawArea = 5;
		rotationFlags = 16;
		lootCategory = "Materials";
		lootTag[] = {"Civilian","Work"};
		itemSize[] = {1,1};
		weight = 1;
		quantityBar = 0;
		canBeSplit = 1;
		isMeleeWeapon = 0;
		absorbency = 1;
	};
	class TraderPlus_Bitcoin: TraderPlusCoin_Base
	{
		scope = 2;
		displayName = "Bitcoin";
		descriptionShort = "a physical Bitcoin 0,03B";
		model = "TraderPlusMoney\Data\money\bitcoin.p3d";
		canBeSplit = 1;
		varQuantityInit = 1;
		varQuantityMin = 0;
		varQuantityMax = 5;
		quantityBar = 0;
	};
	class TraderPlus_Coin: TraderPlusCoin_Base
	{
		scope = 2;
		displayName = "Gold Coin";
		descriptionShort = "Old russian Gold coin";
		model = "TraderPlusMoney\Data\money\coin.p3d";
		canBeSplit = 1;
		varQuantityInit = 1;
		varQuantityMin = 0;
		varQuantityMax = 5;
		quantityBar = 0;
	};
	class TraderPlus_Money_Ruble1_Coin: TraderPlusCoin_Base
	{
		scope = 2;
		displayName = "1 Ruble Coin";
		descriptionShort = "Old russian coins";
		model = "TraderPlusMoney\Data\money\1ruble.p3d";
		varQuantityInit = 1;
		varQuantityMin = 0;
		varQuantityMax = 100;
		varQuantityDestroyOnMin = 1;
		destroyOnEmpty = 1;
	};
	class TraderPlus_Money_Ruble2_Coin: TraderPlusCoin_Base
	{
		scope = 2;
		displayName = "2 Ruble Coin";
		descriptionShort = "Old russian coins";
		model = "TraderPlusMoney\Data\money\2ruble.p3d";
		varQuantityInit = 1;
		varQuantityMin = 0;
		varQuantityMax = 100;
		varQuantityDestroyOnMin = 1;
		destroyOnEmpty = 1;
	};
	class TraderPlus_Money_Ruble5_Coin: TraderPlusCoin_Base
	{
		scope = 2;
		displayName = "5 Ruble Coin";
		descriptionShort = "Old russian coins";
		model = "TraderPlusMoney\Data\money\ruble5.p3d";
		varQuantityInit = 1;
		varQuantityMin = 0;
		varQuantityMax = 100;
		varQuantityDestroyOnMin = 1;
		destroyOnEmpty = 1;
	};
	class TraderPlus_Money_Euro1: TraderPlusCoin_Base
	{
		scope = 2;
		displayName = "1 Euro Coin";
		descriptionShort = "European coins";
		model = "TraderPlusMoney\Data\money\1euro.p3d";
		varQuantityInit = 1;
		varQuantityMin = 0;
		varQuantityMax = 100;
	};
	class TraderPlus_Money_Euro2: TraderPlusCoin_Base
	{
		scope = 2;
		displayName = "2 Euro Coin";
		descriptionShort = "European coins";
		model = "TraderPlusMoney\Data\money\2euro.p3d";
		varQuantityInit = 1;
		varQuantityMin = 0;
		varQuantityMax = 100;
	};
	class TraderPlus_Money_Dollar1: TraderPlusMoney_Base
	{
		scope = 2;
		displayName = "$STR_1USD";
		descriptionShort = "$STR_USD_des";
		model = "TraderPlusMoney\Data\money\1dollar.p3d";
	};
	class TraderPlus_Money_Dollar2: TraderPlusMoney_Base
	{
		scope = 2;
		displayName = "$STR_2USD";
		descriptionShort = "$STR_USD_des";
		model = "TraderPlusMoney\Data\money\2dollar.p3d";
	};
	class TraderPlus_Money_Dollar5: TraderPlusMoney_Base
	{
		scope = 2;
		displayName = "$STR_5USD";
		descriptionShort = "$STR_USD_des";
		model = "TraderPlusMoney\Data\money\5dollar.p3d";
	};
	class TraderPlus_Money_Dollar10: TraderPlusMoney_Base
	{
		scope = 2;
		displayName = "$STR_10USD";
		descriptionShort = "$STR_USD_des";
		model = "TraderPlusMoney\Data\money\10dollar.p3d";
	};
	class TraderPlus_Money_Dollar20: TraderPlusMoney_Base
	{
		scope = 2;
		displayName = "$STR_20USD";
		descriptionShort = "$STR_USD_des";
		model = "TraderPlusMoney\Data\money\20dollar.p3d";
	};
	class TraderPlus_Money_Dollar50: TraderPlusMoney_Base
	{
		scope = 2;
		displayName = "$STR_50USD";
		descriptionShort = "$STR_USD_des";
		model = "TraderPlusMoney\Data\money\50dollar.p3d";
	};
	class TraderPlus_Money_Dollar100: TraderPlusMoney_Base
	{
		scope = 2;
		displayName = "$STR_100USD";
		descriptionShort = "$STR_USD_des";
		model = "TraderPlusMoney\Data\money\100dollar.p3d";
	};
	class TraderPlus_Money_Euro5: TraderPlusMoney_Base
	{
		scope = 2;
		displayName = "$STR_5EUR";
		descriptionShort = "$STR_EUR_des";
		model = "TraderPlusMoney\Data\money\5euro.p3d";
	};
	class TraderPlus_Money_Euro10: TraderPlusMoney_Base
	{
		scope = 2;
		displayName = "$STR_10EUR";
		descriptionShort = "$STR_EUR_des";
		model = "TraderPlusMoney\Data\money\10euro.p3d";
	};
	class TraderPlus_Money_Euro20: TraderPlusMoney_Base
	{
		scope = 2;
		displayName = "$STR_20EUR";
		descriptionShort = "$STR_EUR_des";
		model = "TraderPlusMoney\Data\money\20euro.p3d";
	};
	class TraderPlus_Money_Euro50: TraderPlusMoney_Base
	{
		scope = 2;
		displayName = "$STR_50EUR";
		descriptionShort = "$STR_EUR_des";
		model = "TraderPlusMoney\Data\money\50euro.p3d";
	};
	class TraderPlus_Money_Euro100: TraderPlusMoney_Base
	{
		scope = 2;
		displayName = "$STR_100EUR";
		descriptionShort = "$STR_EUR_des";
		model = "TraderPlusMoney\Data\money\100euro.p3d";
	};
	class TraderPlus_Money_Euro200: TraderPlusMoney_Base
	{
		scope = 2;
		displayName = "$STR_200EUR";
		descriptionShort = "$STR_EUR_des";
		model = "TraderPlusMoney\Data\money\200euro.p3d";
	};
	class TraderPlus_Money_Euro500: TraderPlusMoney_Base
	{
		scope = 2;
		displayName = "$STR_500EUR";
		descriptionShort = "$STR_EUR_des";
		model = "TraderPlusMoney\Data\money\500euro.p3d";
	};
	class TraderPlus_Money_Ruble5: TraderPlusMoney_Base
	{
		scope = 2;
		displayName = "$STR_5RUB";
		descriptionShort = "$STR_RUB_des";
		model = "TraderPlusMoney\Data\money\5ruble.p3d";
	};
	class TraderPlus_Money_Ruble10: TraderPlusMoney_Base
	{
		scope = 2;
		displayName = "$STR_10RUB";
		descriptionShort = "$STR_RUB_des";
		model = "TraderPlusMoney\Data\money\10ruble.p3d";
	};
	class TraderPlus_Money_Ruble50: TraderPlusMoney_Base
	{
		scope = 2;
		displayName = "$STR_50RUB";
		descriptionShort = "$STR_RUB_des";
		model = "TraderPlusMoney\Data\money\50ruble.p3d";
	};
	class TraderPlus_Money_Ruble100: TraderPlusMoney_Base
	{
		scope = 2;
		displayName = "$STR_100RUB";
		descriptionShort = "$STR_RUB_des";
		model = "TraderPlusMoney\Data\money\100ruble.p3d";
	};
	class TraderPlus_Money_Ruble200: TraderPlusMoney_Base
	{
		scope = 2;
		displayName = "$STR_200RUB";
		descriptionShort = "$STR_RUB_des";
		model = "TraderPlusMoney\Data\money\200ruble.p3d";
	};
	class TraderPlus_Money_Ruble500: TraderPlusMoney_Base
	{
		scope = 2;
		displayName = "$STR_500RUB";
		descriptionShort = "$STR_RUB_des";
		model = "TraderPlusMoney\Data\money\500ruble.p3d";
	};
	class TraderPlus_Money_Ruble1000: TraderPlusMoney_Base
	{
		scope = 2;
		displayName = "$STR_1000RUB";
		descriptionShort = "$STR_RUB_des";
		model = "TraderPlusMoney\Data\money\1000ruble.p3d";
	};
	class TraderPlus_Money_Ruble2000: TraderPlusMoney_Base
	{
		scope = 2;
		displayName = "$STR_2000RUB";
		descriptionShort = "$STR_RUB_des";
		model = "TraderPlusMoney\Data\money\2000ruble.p3d";
	};
	class TraderPlus_Money_Ruble5000: TraderPlusMoney_Base
	{
		scope = 2;
		displayName = "$STR_5000RUB";
		descriptionShort = "$STR_RUB_des";
		model = "TraderPlusMoney\Data\money\5000ruble.p3d";
	};
	class TraderPlus_Money_Wad_Ruble5: Wad_Base
	{
		scope = 2;
		displayName = "$STR_5RUB";
		descriptionShort = "$STR_RUB_des";
		model = "TraderPlusMoney\Data\money\wad5.p3d";
		class Resources
		{
			class Money_Ruble5
			{
				value = 100;
				variable = "quantity";
			};
		};
	};
	class TraderPlus_Money_Wad_Ruble10: Wad_Base
	{
		scope = 2;
		displayName = "$STR_10RUB";
		descriptionShort = "$STR_RUB_des";
		model = "TraderPlusMoney\Data\money\wad10.p3d";
		class Resources
		{
			class Money_Ruble10
			{
				value = 100;
				variable = "quantity";
			};
		};
	};
	class TraderPlus_Money_Wad_Ruble50: Wad_Base
	{
		scope = 2;
		displayName = "$STR_50RUB";
		descriptionShort = "$STR_RUB_des";
		model = "TraderPlusMoney\Data\money\wad50.p3d";
		class Resources
		{
			class Money_Ruble50
			{
				value = 50;
				variable = "quantity";
			};
		};
	};
	class TraderPlus_Money_DIM_Gold: TraderPlusCoin_Base
	{
		scope = 2;
		displayName = "$STR_DIM";
		descriptionShort = "$STR_DIM_des";
		model = "TraderPlusMoney\Data\money\DIM_Coin\DIM.p3d";
		varQuantityInit = 1;
		varQuantityMin = 0;
		varQuantityMax = 100;
		varQuantityDestroyOnMin = 1;
		destroyOnEmpty = 1;
		hiddenSelections[] = {"camo"};
		hiddenSelectionsTextures[] = {"\TraderPlusMoney\Data\money\DIM_Coin\Coin_co.paa"};
	};

	// Trader sign of TraderPlus V1: imported TraderPlus traders (and older TraderX samples) use it
	class TraderPlusSign: HouseNoDestruct
	{
		scope = 1;
		model = "\DZ\structures\signs\misc\sign_rail_platformend.p3d";
	};
};
