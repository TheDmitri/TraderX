class TXEnterPasswordResponse
{
    private string _message;
    private bool _isPasswordCorrect;

    void TXEnterPasswordResponse()
    {
        _message = "";
        _isPasswordCorrect = false;
    }

    static ref TXEnterPasswordResponse Create(string message, bool isPasswordCorrect)
    {
        TXEnterPasswordResponse response = new TXEnterPasswordResponse();
        response._message = message;
        response._isPasswordCorrect = isPasswordCorrect;
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

    string ToStringFormat()
    {
        return string.Format("message=%1 isPasswordCorrect=%2", _message, _isPasswordCorrect);
    }
}
