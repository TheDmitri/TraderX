// TraderXGarage - PlayerBase Extension
// Registers the open-garage action.

modded class PlayerBase
{
    override void SetActions()
    {
        super.SetActions();
        AddAction(ActionTXGOpenGarage);
    }
}
