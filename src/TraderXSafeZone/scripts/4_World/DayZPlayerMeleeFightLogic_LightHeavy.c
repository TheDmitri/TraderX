modded class DayZPlayerMeleeFightLogic_LightHeavy
{
    override bool HandleFightLogic(int pCurrentCommandID, HumanInputController pInputs, EntityAI pEntityInHands, HumanMovementState pMovementState, out bool pContinueAttack)
    {
        if (SZHandleFightLogic(pCurrentCommandID, pInputs, pEntityInHands, pMovementState, pContinueAttack))
            return false;

        return super.HandleFightLogic(pCurrentCommandID, pInputs, pEntityInHands, pMovementState, pContinueAttack);
    }

    bool SZHandleFightLogic(int pCurrentCommandID, HumanInputController pInputs, EntityAI pEntityInHands, HumanMovementState pMovementState, out bool pContinueAttack)
    {
        PlayerBase player = PlayerBase.Cast(m_DZPlayer);

        if (player && player.IsInTXSafeZone() && !player.IsTraderXAdmin())
            return true;

        return false;
    }
}
