// TraderXBanking - Action Registration
// Registers the bank opening action with the game's action system

modded class ActionConstructor
{
	override void RegisterActions(TInputActionMap InputActionMap)
	{
		super.RegisterActions(InputActionMap);
		RegisterAction(ActionTXBOpenBank);
	}
}
