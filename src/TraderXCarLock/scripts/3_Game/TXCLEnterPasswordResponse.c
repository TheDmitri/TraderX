// Server -> client result of a password attempt: human message, correctness, optional cooldown.

class TXCLEnterPasswordResponse
{
    string _message;
    bool _isPasswordCorrect;
    ref TXCLCooldownState _cooldownState;

    static TXCLEnterPasswordResponse Create(string message, bool isPwdCorrect, TXCLCooldownState cooldownState)
    {
        TXCLEnterPasswordResponse response = new TXCLEnterPasswordResponse();
        response._message = message;
        response._isPasswordCorrect = isPwdCorrect;
        response._cooldownState = cooldownState;
        return response;
    }

    string GetMessage()
    {
        return _message;
    }

    bool IsPasswordCorrect()
    {
        return _isPasswordCorrect;
    }

    TXCLCooldownState GetCooldownState()
    {
        return _cooldownState;
    }

    string ToStringFormat()
    {
        return string.Format("TXCLEnterPasswordResponse: message=%1, isPasswordCorrect=%2, cooldownState=%3", _message, _isPasswordCorrect, _cooldownState);
    }
}
