/*
=============================================================================
     Program: TraderX Preset Security Service
     File: TraderXPresetSecurityService.c
     Author: Security Enhancement
     Version: v1.0
     Date: 02/09/2025

     Description: Additional security monitoring and logging for preset operations
=============================================================================
*/

class TraderXPresetSecurityService
{
    static ref TraderXPresetSecurityService m_instance;
    private ref map<string, int> m_suspiciousActivityCount;
    private ref array<string> m_blockedPlayers;

    void TraderXPresetSecurityService()
    {
        m_suspiciousActivityCount = new map<string, int>();
        m_blockedPlayers = new array<string>();
    }

    static TraderXPresetSecurityService GetInstance()
    {
        if (!m_instance)
        {
            m_instance = new TraderXPresetSecurityService();
        }
        return m_instance;
    }

    // Monitor and log suspicious preset activities
    void ReportSuspiciousActivity(string playerId, string activityType, string details)
    {
        if (!m_suspiciousActivityCount.Contains(playerId))
        {
            m_suspiciousActivityCount.Set(playerId, 0);
        }

        int currentCount = m_suspiciousActivityCount.Get(playerId);
        currentCount++;
        m_suspiciousActivityCount.Set(playerId, currentCount);

        GetTraderXLogger().LogWarning(string.Format("[SECURITY] Player %1 - %2: %3 (Count: %4)", playerId, activityType, details, currentCount));

        // Auto-block after multiple violations
        if (currentCount >= 5 && m_blockedPlayers.Find(playerId) == -1)
        {
            m_blockedPlayers.Insert(playerId);
            GetTraderXLogger().LogError(string.Format("[SECURITY] Player %1 has been flagged for excessive suspicious activity", playerId));
        }
    }

    // Check if player is flagged for suspicious activity
    bool IsPlayerFlagged(string playerId)
    {
        return m_blockedPlayers.Find(playerId) != -1;
    }

    // Validate preset creation request from client
    bool ValidatePresetCreationRequest(TraderXPreset preset, PlayerIdentity player)
    {
        if (!preset || !player)
            return false;

        string playerId = player.GetId();

        // Check if player is flagged
        if (IsPlayerFlagged(playerId))
        {
            ReportSuspiciousActivity(playerId, "BLOCKED_PRESET_CREATION", "Flagged player attempted preset creation");
            return false;
        }

        // Validate preset structure
        if (!TraderXPresetValidationService.GetInstance().ValidatePresetIntegrity(preset))
        {
            ReportSuspiciousActivity(playerId, "INVALID_PRESET_STRUCTURE", "Preset failed integrity check");
            return false;
        }

        return true;
    }

    // Clear security flags (admin function)
    void ClearPlayerFlags(string playerId)
    {
        m_suspiciousActivityCount.Remove(playerId);
        int idx = m_blockedPlayers.Find(playerId);
        if (idx != -1)
        {
            m_blockedPlayers.Remove(idx);
        }
        GetTraderXLogger().LogInfo(string.Format("[SECURITY] Cleared flags for player %1", playerId));
    }
}
