// Per-lock failed-attempt cooldown. Server computes from CF_Date timestamp; the client
// counts down locally from _remainingSeconds (prepared before sending to client).

class TXCLCooldownState
{
    string _carlockId;
    int _timestamp;
    int _duration;
    int _lastDuration;
    int _remainingSeconds;
    int _clientStartTimeMs;

    void TXCLCooldownState()
    {
        _carlockId = "";
        _timestamp = 0;
        _duration = 0;
        _lastDuration = 0;
        _remainingSeconds = 0;
        _clientStartTimeMs = 0;
    }

    static TXCLCooldownState Create(string carlockId, int duration)
    {
        TXCLCooldownState cooldownState = new TXCLCooldownState();
        cooldownState._carlockId = carlockId;
        cooldownState._lastDuration = duration;
        cooldownState._timestamp = CF_Date.Now().GetTimestamp() + duration;
        return cooldownState;
    }

    void PrepareForClient()
    {
        _remainingSeconds = GetRemainingTime();
        _clientStartTimeMs = 0;
    }

    void InitClientCountdown()
    {
        if (GetGame() && _clientStartTimeMs == 0)
            _clientStartTimeMs = GetGame().GetTime();
    }

    bool HasCooldown()
    {
        if (GetGame() && GetGame().IsServer())
            return _timestamp > CF_Date.Now().GetTimestamp();

        if (_remainingSeconds <= 0)
            return false;

        return GetRemainingTime() > 0;
    }

    int GetRemainingTime()
    {
        if (GetGame() && GetGame().IsServer())
        {
            if (_timestamp <= CF_Date.Now().GetTimestamp())
                return 0;
            return _timestamp - CF_Date.Now().GetTimestamp();
        }

        if (_remainingSeconds <= 0)
            return 0;

        if (GetGame() && _clientStartTimeMs == 0)
            _clientStartTimeMs = GetGame().GetTime();

        int elapsedSeconds = 0;
        if (GetGame())
            elapsedSeconds = (GetGame().GetTime() - _clientStartTimeMs) / 1000;

        int remaining = _remainingSeconds - elapsedSeconds;
        if (remaining < 0)
            remaining = 0;

        return remaining;
    }

    int GetLastDuration()
    {
        return _lastDuration;
    }

    // Exponential back-off: each wrong attempt doubles the wait.
    void AddCooldown()
    {
        _timestamp = CF_Date.Now().GetTimestamp() + _lastDuration * 2;
        _lastDuration *= 2;
    }
}
