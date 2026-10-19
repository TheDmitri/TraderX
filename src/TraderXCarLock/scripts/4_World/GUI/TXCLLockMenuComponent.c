#ifndef CarLock
// TraderPlus V1 defines "CarLock" and owns the CarLock item: TraderX CarLock stands down when it is loaded (see TXCLTraderPlusNotice).
// DayZular keypad for the 8-digit vehicle lock. Two flows:
//  - SET   : owner with no password yet -> type code -> confirm -> LockingRequest (sets pwd).
//  - ENTER : anyone else -> type code -> EnterPasswordAttempt (server validates, cooldown on miss).
// The input is a PASSWORD field: _code holds the real digits, the EditBox only ever shows '*'.
// Physical typing (OnCodeChanged) and the optional on-screen keypad both feed _code.

class TXCLLockMenuComponent extends DZlrComponentBase
{
    private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("tx-carlock-menu", TXCLLockMenuComponent);

    // Bound display strings (member names must match {{ ... }} in the layout).
    string title_text;
    string message_text;

    // Widgets auto-loaded by name from the layout.
    EditBoxWidget codeInput;
    Widget keypadPanel;

    private ref TXCLVehicleLockManager _manager;
    private ref TXCLCooldownState _cooldownState;

    private bool _isSettingPassword;
    private bool _isConfirmingPassword;
    private string _firstPassword;
    private string _code;          // real digits (never shown directly)
    private bool _ignoreClick;
    private bool _keypadVisible;
    private bool _suppressChange;   // guard re-entrancy when we SetText the mask

    const int PASSWORD_LENGTH = 8;
    const string PROMPT_ENTER = "ENTER PASSWORD (8 digits)";
    const string PROMPT_SET = "SET PASSWORD (8 digits)";
    const string PROMPT_CONFIRM = "CONFIRM PASSWORD (8 digits)";
    const string PROMPT_COOLDOWN = "Cooldown: %1s";
    const string ERR_TOO_SHORT = "Password too short!";
    const string ERR_NO_MATCH = "Passwords do not match!";

    override protected void RegisterMetadata()
    {
        m_Metadata = new ComponentMetadata();
        m_Metadata.selector = "tx-carlock-menu";
        m_Metadata.templatePath = "TraderXCarLock/datasets/gui/TXCLLockMenu.layout";
    }

    void SetManager(TXCLVehicleLockManager manager)
    {
        _manager = manager;
    }

    override void OnInit()
    {
        AutoLoadWidgets();

        _isSettingPassword = false;
        _isConfirmingPassword = false;
        _firstPassword = "";
        _code = "";
        _ignoreClick = true;
        _keypadVisible = false;
        _suppressChange = false;

        SetCode("");
        if (keypadPanel)
            keypadPanel.Show(false);

        title_text = "VEHICLE LOCK";
        message_text = PROMPT_ENTER;
        MarkForCheck();

        GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(EnableClicks, 800, false);
        GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(TickCooldown, 1000, true);
    }

    override void OnDestroy()
    {
        GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(EnableClicks);
        GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(TickCooldown);
    }

    void EnableClicks()
    {
        _ignoreClick = false;
    }

    void OnMenuShown()
    {
        InitializeMode();
        RequestCooldownState();
    }

    void InitializeMode()
    {
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player || !_manager)
            return;

        TXCLPlayerDb db = player.GetTXCLPlayerDb();
        _isSettingPassword = db && db.IsOwnerId(_manager.GetVehicleLockOwner()) && !_manager.HasVehicleLockPassword();
        _isConfirmingPassword = false;
        _firstPassword = "";

        SetCode("");
        UpdatePrompt();
    }

    void RequestCooldownState()
    {
        if (GetGame().IsServer() || !_manager)
            return;
        GetRPCManager().SendRPC(TXCL_RPC_NAMESPACE, "GetCooldownState", new Param1<Transport>(_manager.GetTransport()), true);
    }

    // ---- Masked code ---------------------------------------------------------

    string Mask(string value)
    {
        string masked = "";
        for (int i = 0; i < value.Length(); i++)
            masked = masked + "*";
        return masked;
    }

    // Set the real code and render the mask (guarded so the SetText doesn't recurse).
    void SetCode(string value)
    {
        _code = value;
        _suppressChange = true;
        if (codeInput)
            codeInput.SetText(Mask(_code));
        _suppressChange = false;
    }

    void AppendDigits(string chars)
    {
        for (int i = 0; i < chars.Length() && _code.Length() < PASSWORD_LENGTH; i++)
        {
            if (IsDigit(chars[i]))
                _code = _code + chars[i];
        }
    }

    bool IsDigit(string c)
    {
        switch (c)
        {
            case "0": return true;
            case "1": return true;
            case "2": return true;
            case "3": return true;
            case "4": return true;
            case "5": return true;
            case "6": return true;
            case "7": return true;
            case "8": return true;
            case "9": return true;
            default: return false;
        }
        return false;
    }

    // Physical typing into the EditBox. Diff against the current mask length to find what
    // changed, fold it into _code, then re-render the mask.
    void OnCodeChanged()
    {
        if (_suppressChange || !codeInput)
            return;

        string t = codeInput.GetText();
        int prevLen = _code.Length();
        int tLen = t.Length();

        if (tLen > prevLen)
            AppendDigits(t.Substring(prevLen, tLen - prevLen));
        else if (tLen < prevLen)
            _code = _code.Substring(0, tLen);

        SetCode(_code);
    }

    // ---- Keypad toggle ------------------------------------------------------

    void OnToggleKeypad()
    {
        if (_ignoreClick)
            return;
        _keypadVisible = !_keypadVisible;
        if (keypadPanel)
            keypadPanel.Show(_keypadVisible);
    }

    // ---- Keypad handlers (bound via data-on-click) --------------------------

    void OnDigit0() { AddDigit(0); }
    void OnDigit1() { AddDigit(1); }
    void OnDigit2() { AddDigit(2); }
    void OnDigit3() { AddDigit(3); }
    void OnDigit4() { AddDigit(4); }
    void OnDigit5() { AddDigit(5); }
    void OnDigit6() { AddDigit(6); }
    void OnDigit7() { AddDigit(7); }
    void OnDigit8() { AddDigit(8); }
    void OnDigit9() { AddDigit(9); }

    void OnClearClick()
    {
        if (_ignoreClick)
            return;
        SetCode("");
        UpdatePrompt();
    }

    void OnCloseClick()
    {
        if (_ignoreClick)
            return;
        Close();
    }

    void AddDigit(int digit)
    {
        if (_ignoreClick)
            return;
        if (_code.Length() >= PASSWORD_LENGTH)
            return;

        SetCode(_code + digit.ToString());

        if (_code.Length() == PASSWORD_LENGTH)
            UpdatePrompt();
    }

    void OnEnterClick()
    {
        if (_ignoreClick)
            return;

        if (_cooldownState && _cooldownState.HasCooldown())
            return;

        if (_code.Length() != PASSWORD_LENGTH)
        {
            message_text = ERR_TOO_SHORT;
            MarkForCheck();
            return;
        }

        int password = _code.ToInt();

        if (_isSettingPassword)
        {
            if (!_isConfirmingPassword)
            {
                _firstPassword = _code;
                _isConfirmingPassword = true;
                SetCode("");
                message_text = PROMPT_CONFIRM;
                MarkForCheck();
                return;
            }

            if (_firstPassword != _code)
            {
                _firstPassword = "";
                _isConfirmingPassword = false;
                SetCode("");
                message_text = ERR_NO_MATCH;
                MarkForCheck();
                return;
            }

            GetRPCManager().SendRPC(TXCL_RPC_NAMESPACE, "LockingRequest", new Param3<Transport, int, bool>(_manager.GetTransport(), password, true), true);
            Close();
            return;
        }

        GetRPCManager().SendRPC(TXCL_RPC_NAMESPACE, "EnterPasswordAttempt", new Param2<Transport, int>(_manager.GetTransport(), password), true);
    }

    // ---- Prompt -------------------------------------------------------------

    void UpdatePrompt()
    {
        if (_cooldownState && _cooldownState.HasCooldown())
        {
            message_text = string.Format(PROMPT_COOLDOWN, _cooldownState.GetRemainingTime());
            MarkForCheck();
            return;
        }

        if (_isSettingPassword)
        {
            if (_isConfirmingPassword)
                message_text = PROMPT_CONFIRM;
            else
                message_text = PROMPT_SET;
            MarkForCheck();
            return;
        }

        message_text = PROMPT_ENTER;
        MarkForCheck();
    }

    // ---- Cooldown -----------------------------------------------------------

    void SetCooldownState(TXCLCooldownState cooldownState)
    {
        _cooldownState = cooldownState;
        if (_cooldownState)
            _cooldownState.InitClientCountdown();
        UpdatePrompt();
    }

    void TickCooldown()
    {
        if (!_cooldownState)
            return;

        if (_cooldownState.HasCooldown())
        {
            message_text = string.Format(PROMPT_COOLDOWN, _cooldownState.GetRemainingTime());
            MarkForCheck();
        }
        else if (!_isSettingPassword)
        {
            message_text = PROMPT_ENTER;
            MarkForCheck();
        }
    }

    // ---- Server response ----------------------------------------------------

    void OnEnterPasswordResponse(TXCLEnterPasswordResponse response)
    {
        if (!response)
            return;

        SetCode("");
        message_text = response.GetMessage();
        MarkForCheck();

        if (response.GetCooldownState())
            SetCooldownState(response.GetCooldownState());

        if (response.IsPasswordCorrect())
            Close();
    }

    void Close()
    {
        // Defer onto the module (a permanent object) so we never delete this component
        // while still unwinding one of its own click handlers.
        TXCLModule module = GetTXCLModule();
        if (module)
            GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(module.CloseLockMenu, 1, false);
    }
}
#endif
