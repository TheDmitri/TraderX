class CfgPatches
{
	class TraderXGarage_Script
	{
		units[]={};
		weapons[]={};
		requiredVersion=0.1;
		requiredAddons[] = {"DZ_Data","JM_CF_Scripts","TraderX_DayZular"};
	};
};
class CfgMods
{
	class TraderXGarage
	{
		dir="TraderXGarage";
		storageVersion=2;
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
					"TraderXDayZular/scripts/3_Game",
					"TraderX/scripts/3_Game",
					"TraderXGarage/scripts/Common",
					"TraderXGarage/scripts/3_Game"
				};
			};
			class worldScriptModule
			{
				value="";
				files[]=
				{
					"TraderXDayZular/scripts/4_World",
					"TraderX/scripts/4_World",
					"TraderXGarage/scripts/Common",
					"TraderXGarage/scripts/4_World"
				};
			};
			class missionScriptModule
			{
				value="";
				files[]=
				{
					"TraderXDayZular/scripts/5_Mission",
					"TraderX/scripts/5_Mission",
					"TraderXGarage/scripts/Common",
					"TraderXGarage/scripts/5_Mission"
				};
			};
		};
	};
};
class CfgVehicles
{
	class Paper;
	class Wrench;
	// TraderPlus V1 vehicle receipts (same classes, so receipts held by players are kept)
	class ReceiptBase: Paper
	{
		scope = 2;
		displayName = "$STR_txg_receipt";
		descriptionShort = "$STR_txg_receipt_des";
		forceFarBubble = "true";
		weight = 1000;
		itemSize[] = {1,1};
		rotationFlags = 0;
		itemBehaviour = 2;
		canBeDigged = 0;
		heavyItem = 1;
		physLayer = "item_large";
		bounding = "BSphere";
		carveNavmesh = 1;
		slopeTolerance = 0.2;
		yawPitchRollLimit[] = {12,12,12};
		placement = "ForceSlopeOnTerrain";
		canBeSplit = 0;
		varQuantityInit = 1;
		varQuantityMin = 0;
		varQuantityMax = 1;
		varQuantityDestroyOnMin = 1;
		varStackMax = 0;
		itemsCargoSize[] = {10,100};
		openable = 0;
		allowOwnedCargoManipulation = 1;
	};
	class Receipt: ReceiptBase
	{
		scope = 2;
	};
	class NewReceiptBase: Paper
	{
		scope = 2;
		displayName = "$STR_txg_receipt";
		descriptionShort = "$STR_txg_receipt_des";
		forceFarBubble = "true";
		weight = 1000;
		itemSize[] = {1,1};
		rotationFlags = 0;
		itemBehaviour = 2;
		canBeDigged = 0;
		heavyItem = 1;
		physLayer = "item_large";
		bounding = "BSphere";
		carveNavmesh = 1;
		slopeTolerance = 0.2;
		yawPitchRollLimit[] = {12,12,12};
		placement = "ForceSlopeOnTerrain";
		canBeSplit = 0;
		varQuantityInit = 1;
		varQuantityMin = 0;
		varQuantityMax = 1;
		varQuantityDestroyOnMin = 1;
		varStackMax = 0;
		itemsCargoSize[] = {10,100};
		openable = 0;
		allowOwnedCargoManipulation = 1;
	};
	class NewReceipt: NewReceiptBase
	{
		scope = 2;
	};
	class TraderPlusWrench: Wrench
	{
		scope = 2;
		displayName = "$STR_txg_packing_wrench";
		descriptionShort = "$STR_txg_packing_wrench_des";
	};
};
