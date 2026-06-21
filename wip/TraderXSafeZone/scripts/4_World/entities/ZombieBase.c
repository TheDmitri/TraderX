modded class ZombieBase
{
    bool DeleteZombieHandler()
    {
        PlayerBase player = PlayerBase.Cast(m_ActualTarget);
        if(player && player.IsInTXSafeZone())
        {
            GetGame().ObjectDelete(this);
            return true;
        }

        return false;
    }

    override bool ChaseAttackLogic(int pCurrentCommandID, DayZInfectedInputController pInputController, float pDt)
    {
        if(DeleteZombieHandler())
            return false;

        return super.ChaseAttackLogic(pCurrentCommandID, pInputController, pDt);
    }

    override bool FightAttackLogic(int pCurrentCommandID, DayZInfectedInputController pInputController, float pDt)
    {
        if(DeleteZombieHandler())
            return false;

        return super.FightAttackLogic(pCurrentCommandID, pInputController, pDt);
    }
}
