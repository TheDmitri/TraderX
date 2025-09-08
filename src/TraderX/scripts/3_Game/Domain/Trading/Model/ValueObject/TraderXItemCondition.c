// In 3_Game/Domain/Trading/ValueObjects/TraderXItemCondition.c
class TraderXItemCondition
{
    // Constants for condition thresholds
    static const float DAMAGED_THRESHOLD = 0.75;
    static const float BADLY_DAMAGED_THRESHOLD = 0.45;
    static const float RUINED_THRESHOLD = 0.0;

    // Condition states
    private bool m_IsPristine;
    private bool m_IsWorn;
    private bool m_IsDamaged;
    private bool m_IsBadlyDamaged;
    private bool m_IsRuined;
    
    // Coefficients for value calculation
    private float m_ConditionCoefficient;
    private float m_HealthLevel; // 0.0 to 1.0

    void TraderXItemCondition(float healthLevel)
    {
        // Validate input
        m_HealthLevel = Math.Clamp(healthLevel, 0.0, 1.0);
        UpdateConditionState();
        CalculateConditionCoefficient();
    }

    // Getters for condition states
    bool IsPristine() { return m_IsPristine; }
    bool IsWorn() { return m_IsWorn; }
    bool IsDamaged() { return m_IsDamaged; }
    bool IsBadlyDamaged() { return m_IsBadlyDamaged; }
    bool IsRuined() { return m_IsRuined; }
    
    // Get the condition coefficient for value calculation
    float GetValueCoefficient() { return m_ConditionCoefficient; }
    
    // Get the condition as a string
    string GetConditionText()
    {
        if (m_IsRuined) return "Ruined";
        if (m_IsBadlyDamaged) return "Badly Damaged";
        if (m_IsDamaged) return "Damaged";
        if (m_IsWorn) return "Worn";
        return "Pristine";
    }

    // Check if item is in a tradable condition based on settings
    bool IsTradable(TraderXStates traderStates)
    {
        if (m_IsRuined) return false;
        if (m_IsBadlyDamaged) return traderStates.acceptBadlyDamaged;
        if (m_IsDamaged) return traderStates.acceptDamaged;
        if (m_IsWorn) return traderStates.acceptWorn;
        return true; // Pristine is always tradable
    }

    // Calculate the trade value based on condition
    float CalculateTradeValue(float baseValue, TraderXStates traderStates)
    {
        if (!IsTradable(traderStates)) return 0.0;
        return baseValue * m_ConditionCoefficient;
    }

    // Private methods
    private void UpdateConditionState()
    {
        m_IsPristine = (m_HealthLevel >= 0.95);
        m_IsWorn = (m_HealthLevel >= DAMAGED_THRESHOLD && m_HealthLevel < 0.95);
        m_IsDamaged = (m_HealthLevel >= BADLY_DAMAGED_THRESHOLD && m_HealthLevel < DAMAGED_THRESHOLD);
        m_IsBadlyDamaged = (m_HealthLevel > RUINED_THRESHOLD && m_HealthLevel < BADLY_DAMAGED_THRESHOLD);
        m_IsRuined = (m_HealthLevel <= RUINED_THRESHOLD);
    }

    private void CalculateConditionCoefficient()
    {
        if (m_IsRuined) 
        {
            m_ConditionCoefficient = 0.0;
        }
        else if (m_IsBadlyDamaged)
        {
            // Scale between 0.25 and 0.49 based on health
            m_ConditionCoefficient = 0.25 + (0.24 * ((m_HealthLevel - RUINED_THRESHOLD) / (BADLY_DAMAGED_THRESHOLD - RUINED_THRESHOLD)));
        }
        else if (m_IsDamaged)
        {
            // Scale between 0.5 and 0.74
            m_ConditionCoefficient = 0.5 + (0.24 * ((m_HealthLevel - BADLY_DAMAGED_THRESHOLD) / (DAMAGED_THRESHOLD - BADLY_DAMAGED_THRESHOLD)));
        }
        else if (m_IsWorn)
        {
            // Scale between 0.75 and 0.94
            m_ConditionCoefficient = 0.75 + (0.19 * ((m_HealthLevel - DAMAGED_THRESHOLD) / (0.95 - DAMAGED_THRESHOLD)));
        }
        else
        {
            // Pristine (0.95 to 1.0)
            m_ConditionCoefficient = 0.95 + (0.05 * ((m_HealthLevel - 0.95) / 0.05));
        }
    }
}