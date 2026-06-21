modded class PlayerBase
{
    private bool netSync_IsInTXSafeZone = false;
    private bool netSync_IsAdmin = false;
    private int m_InfluenzaLevelBeforeSafeZone = 0;

    override void Init()
    {
        super.Init();
        RegisterNetSyncVariableBool("netSync_IsInTXSafeZone");
        RegisterNetSyncVariableBool("netSync_IsAdmin");
    }

    void SetTXSafeZoneStatus(bool isInSZ)
    {
        netSync_IsInTXSafeZone = isInSZ;
        SetSynchDirty();
    }

    bool IsInTXSafeZone()
    {
        return netSync_IsInTXSafeZone;
    }

    void SetTXAdminStatus(bool isAdmin)
    {
        netSync_IsAdmin = isAdmin;
        SetSynchDirty();
    }

    bool IsTraderXAdmin()
    {
        return netSync_IsAdmin;
    }

    void SaveInfluenzaLevel()
    {
        if(m_AgentPool)
            m_InfluenzaLevelBeforeSafeZone = m_AgentPool.GetSingleAgentCount(eAgents.INFLUENZA);
    }

    void ClearInfluenza()
    {
        if(m_AgentPool)
            m_AgentPool.SetAgentCount(eAgents.INFLUENZA, 0);
    }

    void RestoreInfluenza()
    {
        if(m_AgentPool)
            m_AgentPool.SetAgentCount(eAgents.INFLUENZA, m_InfluenzaLevelBeforeSafeZone);
    }

    override void EOnFrame(IEntity other, float timeSlice)
    {
        if (IsControlledPlayer() && IsInTXSafeZone())
            GetUApi().GetInputByID(UATempRaiseWeapon).Supress();

        super.EOnFrame(other, timeSlice);
    }

    override void CommandHandler(float pDt, int pCurrentCommandID, bool pCurrentCommandFinished)
    {
        if (IsControlledPlayer() && IsInTXSafeZone())
        {
            GetUApi().GetInputByID(UATempRaiseWeapon).Supress();
            if (GetWeaponManager())
            {
                AbortWeaponEvent();
                GetWeaponManager().DelayedRefreshAnimationState(10);
            }
            if (IsFireWeaponRaised())
                ExitSights();
        }
        
        super.CommandHandler(pDt, pCurrentCommandID, pCurrentCommandFinished);
    }
}