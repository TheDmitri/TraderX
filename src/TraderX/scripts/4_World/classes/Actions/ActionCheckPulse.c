modded class ActionCheckPulse: ActionInteractBase
{
	override bool ActionCondition( PlayerBase player, ActionTarget target, ItemBase item )
	{
		if(!super.ActionCondition(player, target, item))
			return false;

		PlayerBase ntarget = PlayerBase.Cast(  target.GetObject() );
		if( ntarget && ntarget.IsAlive() && ntarget.IsTraderNpc()){
			return false;
		}

		return true;
	}
}
