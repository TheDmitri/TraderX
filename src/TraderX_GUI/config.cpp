class CfgMods
{
	class TraderX_GUI
	{
		dir="TraderX_GUI";
		picture="";
		action="";
		hideName=1;
		hidePicture=1;
		name="TraderX_GUI";
		author="Malinin | Dmitri";
		version="Version 1.0";
		extra=0;
		type="mod";
		class defs
		{
			class widgetStyles
			{
				files[]=
				{
					"TraderX_GUI/gui/Styles.styles"
				};
			};
			class imageSets
			{
				files[]=
				{
					"TraderX_GUI/gui/Data/first.imageset",
					"TraderX_GUI/gui/Data/square_shadow.imageset"
				};
			};
		};
	};
};
class CfgPatches
{
	class TraderX_GUI
	{
		units[]={};
		weapons[]={};
		requiredVersion=0.1;
		requiredAddons[]=
		{
			"RPC_Scripts"
		};
	};
};