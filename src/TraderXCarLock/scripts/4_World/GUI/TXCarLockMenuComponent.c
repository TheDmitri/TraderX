class TXCarLockMenuComponent extends DZlrComponentBase
{
    private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("tx-carlock-menu", TXCarLockMenuComponent);

    // Bound properties
    string statusMessage;
    string codeDisplayText;

    // Internal state
    private ref VehicleLockManager _manager;
    private string _enteredCode;
    private bool _isSettingPassword;
    private bool _isConfirmingPassword;
    private string _firstPassword;

    override protected void RegisterMetadata()
    {
        m_Metadata = new ComponentMetadata();
        m_Metadata.selector = "tx-carlock-menu";
        m_Metadata.templatePath = "TraderXCarLock/datasets/gui/TXCarLockMenu.layout";
    }

    void SetManager(VehicleLockManager manager)
    {
        _manager = manager;
    }

    override void OnInit()
    {
        _enteredCode = "";
        _isSettingPassword = false;
        _isConfirmingPassword = false;
        _firstPassword = "";

        InitializeMenu();
        UpdateCodeDisplay();
        MarkForCheck();
    }

    override void OnDestroy()
    {
    }

    void InitializeMenu()
    {
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());

        if (!player || !_manager)
            return;

        _isSettingPassword = player.GetCarLockDb().IsOwnerId(_manager.GetVehicleLockOwner()) && !_manager.HasVehicleLockPassword();
        _isConfirmingPassword = false;
        _firstPassword = "";

        if (_isSettingPassword)
        {
            statusMessage = string.Format("SET PASSWORD (%1)", TX_CL_PASSWORD_LENGTH);
        }
        else
        {
            statusMessage = string.Format("ENTER PASSWORD (%1)", TX_CL_PASSWORD_LENGTH);
        }
    }

    void UpdateCodeDisplay()
    {
        string maskedCode = "";
        for (int i = 0; i < _enteredCode.Length(); i++)
        {
            maskedCode += "*";
        }
        codeDisplayText = maskedCode;
    }

    // === Digit click handlers ===

    void OnDigit0Click() { AddDigit("0"); }
    void OnDigit1Click() { AddDigit("1"); }
    void OnDigit2Click() { AddDigit("2"); }
    void OnDigit3Click() { AddDigit("3"); }
    void OnDigit4Click() { AddDigit("4"); }
    void OnDigit5Click() { AddDigit("5"); }
    void OnDigit6Click() { AddDigit("6"); }
    void OnDigit7Click() { AddDigit("7"); }
    void OnDigit8Click() { AddDigit("8"); }
    void OnDigit9Click() { AddDigit("9"); }

    void AddDigit(string digit)
    {
        if (_enteredCode.Length() >= TX_CL_PASSWORD_LENGTH)
            return;

        _enteredCode += digit;
        UpdateCodeDisplay();
        MarkForCheck();
    }

    void OnClearClick()
    {
        _enteredCode = "";
        UpdateCodeDisplay();
        UpdateBasePromptMessage();
        MarkForCheck();
    }

    void OnCloseClick()
    {
        GetTXCarLockUIService().CloseCarLockMenu();
    }

    void OnEnterClick()
    {
        if (CheckPasswordForErrors(_enteredCode))
            return;

        int password = _enteredCode.ToInt();

        if (_isSettingPassword)
        {
            if (!_isConfirmingPassword)
            {
                _firstPassword = _enteredCode;
                _isConfirmingPassword = true;
                _enteredCode = "";
                statusMessage = string.Format("CONFIRM PASSWORD (%1)", TX_CL_PASSWORD_LENGTH);
                UpdateCodeDisplay();
                MarkForCheck();
                return;
            }

            if (_firstPassword != _enteredCode)
            {
                _firstPassword = "";
                _isConfirmingPassword = false;
                _enteredCode = "";
                statusMessage = "Passwords do not match!";
                UpdateCodeDisplay();
                MarkForCheck();
                return;
            }

            GetDayZularRPCManager().SendRPC("TXCarLock", "GetCarLockLockingRequest", new Param3<Transport, int, bool>(_manager.GetTransport(), password, true), true);
            GetTXCarLockUIService().CloseCarLockMenu();
            return;
        }

        GetDayZularRPCManager().SendRPC("TXCarLock", "EnterPasswordAttempt", new Param2<Transport, int>(_manager.GetTransport(), password), true);
        _enteredCode = "";
        UpdateCodeDisplay();
        MarkForCheck();
    }

    void UpdateBasePromptMessage()
    {
        if (_isSettingPassword)
        {
            if (_isConfirmingPassword)
                statusMessage = string.Format("CONFIRM PASSWORD (%1)", TX_CL_PASSWORD_LENGTH);
            else
                statusMessage = string.Format("SET PASSWORD (%1)", TX_CL_PASSWORD_LENGTH);
        }
        else
        {
            statusMessage = string.Format("ENTER PASSWORD (%1)", TX_CL_PASSWORD_LENGTH);
        }
    }

    bool HasCharacterInPassword(string strPassword)
    {
        for (int i = 0; i < strPassword.Length(); i++)
        {
            switch (strPassword[i])
            {
                case "0": continue;
                case "1": continue;
                case "2": continue;
                case "3": continue;
                case "4": continue;
                case "5": continue;
                case "6": continue;
                case "7": continue;
                case "8": continue;
                case "9": continue;
                default: return true;
            }
        }

        return false;
    }

    bool CheckPasswordForErrors(string strPassword)
    {
        if (strPassword.Length() < TX_CL_PASSWORD_LENGTH)
        {
            statusMessage = "Password too short!";
            MarkForCheck();
            return true;
        }

        if (strPassword.Length() > TX_CL_PASSWORD_LENGTH)
        {
            statusMessage = "Password too long!";
            MarkForCheck();
            return true;
        }

        if (HasCharacterInPassword(strPassword))
        {
            statusMessage = "Password contains character!";
            MarkForCheck();
            return true;
        }

        return false;
    }

    void OnEnterPasswordAttemptResponse(TXEnterPasswordResponse response)
    {
        statusMessage = response.GetMessage();
        _enteredCode = "";
        UpdateCodeDisplay();
        MarkForCheck();

        GetTXCarLockLogger().LogDebug("OnEnterPasswordAttemptResponse: " + response.ToStringFormat());

        if (response.IsPasswordCorrect())
        {
            GetTXCarLockUIService().CloseCarLockMenu();
        }
    }
}
