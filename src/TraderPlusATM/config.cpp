////////////////////////////////////////////////////////////////////
//DeRap: config.bin
//Produced from mikero's Dos Tools Dll version 10.13
//https://mikero.bytex.digital/Downloads
//'now' is Mon Jun 22 11:42:27 2026 : 'file' last modified on Mon Jun 22 11:42:27 2026
////////////////////////////////////////////////////////////////////

#define _ARMA_

class CfgPatches
{
	class TraderPlus_ATM
	{
		units[] = {};
		weapons[] = {};
		requiredVersion = 0.1;
		requiredAddons[] = {"DZ_Data"};
	};
};
class CfgMods
{
	class TraderPlus_ATM
	{
		dir = "TraderPlusATM";
		picture = "";
		action = "";
		hideName = 1;
		hidePicture = 1;
		name = "TraderPlusATM";
		credits = "Morty";
		author = "Morty";
		authorID = "0";
		version = "1.0";
		extra = 0;
		type = "mod";
		dependencies[] = {"World"};
		class defs
		{
			class worldScriptModule
			{
				value = "";
				files[] = {"TraderPlusATM/Scripts/4_World"};
			};
		};
	};
};
// Merged from the former ATM/config.cpp - kept here so the PBO has a single root config
// (two configs both declaring CfgPatches.TraderPlus_ATM caused "Conflicting addon" -> invisible ATM).
class CfgVehicles
{
	class HouseNoDestruct;
	class TraderPlus_BANK_ATM: HouseNoDestruct
	{
		scope = 1;
		model = "\TraderPlusATM\ATM\TraderPlus_ATM.p3d";
		hiddenSelections[] = {"box","screen"};
		hiddenSelectionsTextures[] = {"\TraderPlusATM\ATM\data\ATM_co.paa","\TraderPlusATM\ATM\data\ATM_SCREEN_co.paa"};
	};
};
