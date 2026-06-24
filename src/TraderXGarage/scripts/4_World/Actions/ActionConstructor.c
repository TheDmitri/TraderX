// TraderXGarage - Action Registration
// Registers the open-garage action with the game's action system.

modded class ActionConstructor
{
    override void RegisterActions(TTypenameArray actions)
    {
        super.RegisterActions(actions);
        actions.Insert(ActionTXGOpenGarage);
    }
}
