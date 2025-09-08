class CfgPatches {
	class money {
		units[] = {"TraderX_Wallet", "TraderX_Bitcoin", "TraderX_Coin", "TraderX_Money_Ruble1_Coin", "TraderX_Money_Ruble2_Coin", "TraderX_Money_Ruble5_Coin", "TraderX_Money_Euro1", "TraderX_Money_Euro2", "TraderX_Money_Dollar1", "TraderX_Money_Dollar2", "TraderX_Money_Dollar5", "TraderX_Money_Dollar10", "TraderX_Money_Dollar20", "TraderX_Money_Dollar50", "TraderX_Money_Dollar100", "TraderX_Money_Euro5", "TraderX_Money_Euro10", "TraderX_Money_Euro20", "TraderX_Money_Euro50", "TraderX_Money_Euro100", "TraderX_Money_Euro200", "TraderX_Money_Euro500", "TraderX_Money_Ruble5", "TraderX_Money_Ruble10", "TraderX_Money_Ruble50", "TraderX_Money_Ruble100", "TraderX_Money_Ruble200", "TraderX_Money_Ruble500", "TraderX_Money_Ruble1000", "TraderX_Money_Ruble2000", "TraderX_Money_Ruble5000", "TraderX_Money_Wad_Ruble5", "TraderX_Money_Wad_Ruble10", "TraderX_Money_Wad_Ruble50", "TraderX_Money_DIM_Gold"};
		weapons[] = {};
		requiredVersion = 0.1;
		requiredAddons[] = {"DZ_Data"};
	};
};

class CfgVehicles {
	class Inventory_Base;	// External class reference
	
	class TraderX_Wallet : Inventory_Base {
		scope = 2;
		displayName = "$STR_Wallet";
		descriptionShort = "$STR_Wallet_des";
		model = "TraderPlusMoney\Data\Wallet\Wallet.p3d";
		weight = 130;
		itemSize[] = {2, 2};
		itemsCargoSize[] = {6, 4};
		canBeDigged = 0;
		allowOwnedCargoManipulation = 1;
	};
	
	class TraderXCoin_Base : Inventory_Base {
		scope = 0;
		overrideDrawArea = 5.0;
		rotationFlags = 16;
		lootCategory = "Materials";
		lootTag[] = {"Civilian", "Work"};
		itemSize[] = {1, 1};
		weight = 1;
		quantityBar = 0;
		canBeSplit = 1;
		isMeleeWeapon = 0;
		absorbency = 1;
	};
	
	class TraderX_Bitcoin : TraderXCoin_Base {
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
	
	class TraderX_Coin : TraderXCoin_Base {
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
	
	class TraderX_Money_Ruble1_Coin : TraderXCoin_Base {
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
	
	class TraderX_Money_Ruble2_Coin : TraderXCoin_Base {
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
	
	class TraderX_Money_Ruble5_Coin : TraderXCoin_Base {
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
	
	class TraderX_Money_Euro1 : TraderXCoin_Base {
		scope = 2;
		displayName = "1 Euro Coin";
		descriptionShort = "European coins";
		model = "TraderPlusMoney\Data\money\1euro.p3d";
		varQuantityInit = 1;
		varQuantityMin = 0;
		varQuantityMax = 100;
	};
	
	class TraderX_Money_Euro2 : TraderXCoin_Base {
		scope = 2;
		displayName = "2 Euro Coin";
		descriptionShort = "European coins";
		model = "TraderPlusMoney\Data\money\2euro.p3d";
		varQuantityInit = 1;
		varQuantityMin = 0;
		varQuantityMax = 100;
	};
	
	class TraderPlusMoney_Base : Inventory_Base {
		scope = 0;
		overrideDrawArea = 5.0;
		rotationFlags = 16;
		lootCategory = "Materials";
		lootTag[] = {"Civilian", "Work"};
		itemSize[] = {2, 1};
		weight = 1;
		quantityBar = 0;
		canBeSplit = 1;
		varQuantityInit = 1;
		varQuantityMin = 0;
		varQuantityMax = 500;
		varQuantityDestroyOnMin = 1;
		destroyOnEmpty = 1;
		isMeleeWeapon = 0;
		absorbency = 1;
	};
	
	class TraderX_Money_Dollar1 : TraderPlusMoney_Base {
		scope = 2;
		displayName = "$STR_1USD";
		descriptionShort = "$STR_USD_des";
		model = "TraderPlusMoney\Data\money\1dollar.p3d";
	};
	
	class TraderX_Money_Dollar2 : TraderPlusMoney_Base {
		scope = 2;
		displayName = "$STR_2USD";
		descriptionShort = "$STR_USD_des";
		model = "TraderPlusMoney\Data\money\2dollar.p3d";
	};
	
	class TraderX_Money_Dollar5 : TraderPlusMoney_Base {
		scope = 2;
		displayName = "$STR_5USD";
		descriptionShort = "$STR_USD_des";
		model = "TraderPlusMoney\Data\money\5dollar.p3d";
	};
	
	class TraderX_Money_Dollar10 : TraderPlusMoney_Base {
		scope = 2;
		displayName = "$STR_10USD";
		descriptionShort = "$STR_USD_des";
		model = "TraderPlusMoney\Data\money\10dollar.p3d";
	};
	
	class TraderX_Money_Dollar20 : TraderPlusMoney_Base {
		scope = 2;
		displayName = "$STR_20USD";
		descriptionShort = "$STR_USD_des";
		model = "TraderPlusMoney\Data\money\20dollar.p3d";
	};
	
	class TraderX_Money_Dollar50 : TraderPlusMoney_Base {
		scope = 2;
		displayName = "$STR_50USD";
		descriptionShort = "$STR_USD_des";
		model = "TraderPlusMoney\Data\money\50dollar.p3d";
	};
	
	class TraderX_Money_Dollar100 : TraderPlusMoney_Base {
		scope = 2;
		displayName = "$STR_100USD";
		descriptionShort = "$STR_USD_des";
		model = "TraderPlusMoney\Data\money\100dollar.p3d";
	};
	
	class TraderX_Money_Euro5 : TraderPlusMoney_Base {
		scope = 2;
		displayName = "$STR_5EUR";
		descriptionShort = "$STR_EUR_des";
		model = "TraderPlusMoney\Data\money\5euro.p3d";
	};
	
	class TraderX_Money_Euro10 : TraderPlusMoney_Base {
		scope = 2;
		displayName = "$STR_10EUR";
		descriptionShort = "$STR_EUR_des";
		model = "TraderPlusMoney\Data\money\10euro.p3d";
	};
	
	class TraderX_Money_Euro20 : TraderPlusMoney_Base {
		scope = 2;
		displayName = "$STR_20EUR";
		descriptionShort = "$STR_EUR_des";
		model = "TraderPlusMoney\Data\money\20euro.p3d";
	};
	
	class TraderX_Money_Euro50 : TraderPlusMoney_Base {
		scope = 2;
		displayName = "$STR_50EUR";
		descriptionShort = "$STR_EUR_des";
		model = "TraderPlusMoney\Data\money\50euro.p3d";
	};
	
	class TraderX_Money_Euro100 : TraderPlusMoney_Base {
		scope = 2;
		displayName = "$STR_100EUR";
		descriptionShort = "$STR_EUR_des";
		model = "TraderPlusMoney\Data\money\100euro.p3d";
	};
	
	class TraderX_Money_Euro200 : TraderPlusMoney_Base {
		scope = 2;
		displayName = "$STR_200EUR";
		descriptionShort = "$STR_EUR_des";
		model = "TraderPlusMoney\Data\money\200euro.p3d";
	};
	
	class TraderX_Money_Euro500 : TraderPlusMoney_Base {
		scope = 2;
		displayName = "$STR_500EUR";
		descriptionShort = "$STR_EUR_des";
		model = "TraderPlusMoney\Data\money\500euro.p3d";
	};
	
	class TraderX_Money_Ruble5 : TraderPlusMoney_Base {
		scope = 2;
		displayName = "$STR_5RUB";
		descriptionShort = "$STR_RUB_des";
		model = "TraderPlusMoney\Data\money\5ruble.p3d";
	};
	
	class TraderX_Money_Ruble10 : TraderPlusMoney_Base {
		scope = 2;
		displayName = "$STR_10RUB";
		descriptionShort = "$STR_RUB_des";
		model = "TraderPlusMoney\Data\money\10ruble.p3d";
	};
	
	class TraderX_Money_Ruble50 : TraderPlusMoney_Base {
		scope = 2;
		displayName = "$STR_50RUB";
		descriptionShort = "$STR_RUB_des";
		model = "TraderPlusMoney\Data\money\50ruble.p3d";
	};
	
	class TraderX_Money_Ruble100 : TraderPlusMoney_Base {
		scope = 2;
		displayName = "$STR_100RUB";
		descriptionShort = "$STR_RUB_des";
		model = "TraderPlusMoney\Data\money\100ruble.p3d";
	};
	
	class TraderX_Money_Ruble200 : TraderPlusMoney_Base {
		scope = 2;
		displayName = "$STR_200RUB";
		descriptionShort = "$STR_RUB_des";
		model = "TraderPlusMoney\Data\money\200ruble.p3d";
	};
	
	class TraderX_Money_Ruble500 : TraderPlusMoney_Base {
		scope = 2;
		displayName = "$STR_500RUB";
		descriptionShort = "$STR_RUB_des";
		model = "TraderPlusMoney\Data\money\500ruble.p3d";
	};
	
	class TraderX_Money_Ruble1000 : TraderPlusMoney_Base {
		scope = 2;
		displayName = "$STR_1000RUB";
		descriptionShort = "$STR_RUB_des";
		model = "TraderPlusMoney\Data\money\1000ruble.p3d";
	};
	
	class TraderX_Money_Ruble2000 : TraderPlusMoney_Base {
		scope = 2;
		displayName = "$STR_2000RUB";
		descriptionShort = "$STR_RUB_des";
		model = "TraderPlusMoney\Data\money\2000ruble.p3d";
	};
	
	class TraderX_Money_Ruble5000 : TraderPlusMoney_Base {
		scope = 2;
		displayName = "$STR_5000RUB";
		descriptionShort = "$STR_RUB_des";
		model = "TraderPlusMoney\Data\money\5000ruble.p3d";
	};
	class Box_Base;	// External class reference
	
	class Wad_Base : Box_Base {
		scope = 0;
		weight = 2540;
		absorbency = 0.8;
		itemSize[] = {2, 1};
		
		class DamageSystem {
			class GlobalHealth {
				class Health {
					hitpoints = 80;
					healthLevels[] = {{1, {"TraderPlusMoney\Data\money\textures\Money.rvmat"}}, {0.7, {"TraderPlusMoney\Data\money\textures\Money.rvmat"}}, {0.5, {"TraderPlusMoney\Data\money\textures\Money_damage.rvmat"}}, {0.3, {"TraderPlusMoney\Data\money\textures\Money_damage.rvmat"}}, {0, {"TraderPlusMoney\Data\money\textures\Money_destruct.rvmat"}}};
				};
			};
		};
		
		class AnimEvents {
			class SoundWeapon {
				class interact {
					soundset = "ammoboxUnpack_SoundSet";
					id = 70;
				};
			};
		};
	};
	
	class TraderX_Money_Wad_Ruble5 : Wad_Base {
		scope = 2;
		displayName = "$STR_5RUB";
		descriptionShort = "$STR_RUB_des";
		model = "TraderPlusMoney\Data\money\wad5.p3d";
		
		class Resources {
			class Money_Ruble5 {
				value = 100;
				variable = "quantity";
			};
		};
	};
	
	class TraderX_Money_Wad_Ruble10 : Wad_Base {
		scope = 2;
		displayName = "$STR_10RUB";
		descriptionShort = "$STR_RUB_des";
		model = "TraderPlusMoney\Data\money\wad10.p3d";
		
		class Resources {
			class Money_Ruble10 {
				value = 100;
				variable = "quantity";
			};
		};
	};
	
	class TraderX_Money_Wad_Ruble50 : Wad_Base {
		scope = 2;
		displayName = "$STR_50RUB";
		descriptionShort = "$STR_RUB_des";
		model = "TraderPlusMoney\Data\money\wad50.p3d";
		
		class Resources {
			class Money_Ruble50 {
				value = 50;
				variable = "quantity";
			};
		};
	};
	
	class TraderX_Money_DIM_Gold : TraderXCoin_Base {
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
};
