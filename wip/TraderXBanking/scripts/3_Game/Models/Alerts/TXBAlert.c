// TraderXBanking - Alert Model
// Generic alert/notification for dashboard display.
// Supports multiple alert types with appropriate styling.

class TXBAlert
{
    // Alert types
    static const int TYPE_INFO = 0;
    static const int TYPE_SUCCESS = 1;
    static const int TYPE_WARNING = 2;
    static const int TYPE_DANGER = 3;
    static const int TYPE_CLAN = 4;
    
    // Data
    int type;
    string title;
    string body;
    string actionLabel;
    string actionTarget;
    int priority;
    int expiresAt;
    bool isDismissible;
    string alertId;
    
    void TXBAlert()
    {
        type = TYPE_INFO;
        title = "";
        body = "";
        actionLabel = "";
        actionTarget = "";
        priority = 0;
        expiresAt = 0;
        isDismissible = true;
        alertId = "";
    }
    
    // Factory: Vault fee due alert
    static TXBAlert VaultFeeDue(int amount, int hoursRemaining)
    {
        TXBAlert alert = new TXBAlert();
        alert.type = TYPE_WARNING;
        alert.priority = 100;
        
        alert.title = "Vault fee due in " + TXBFormat.TimeRemaining(hoursRemaining);
        alert.body = TXBFormat.Currency(amount) + " will be auto-debited from your bank balance. Your balance is sufficient.";
        alert.alertId = "vault_fee_" + hoursRemaining.ToString();
        
        return alert;
    }
    
    // Factory: Vault frozen alert (fees unpaid)
    static TXBAlert VaultFrozen(int amountOwed)
    {
        TXBAlert alert = new TXBAlert();
        alert.type = TYPE_DANGER;
        alert.priority = 200;
        
        alert.title = "VAULT FROZEN";
        alert.body = "Fees unpaid: " + TXBFormat.Currency(amountOwed) + " owed. Deposit funds to unfreeze your vault.";
        alert.actionLabel = "DEPOSIT →";
        alert.actionTarget = "banking-account-tab";
        alert.alertId = "vault_frozen";
        
        return alert;
    }
    
    // Factory: Clan invite received
    static TXBAlert ClanInvite(string clanName, string invitedBy, int memberCount)
    {
        TXBAlert alert = new TXBAlert();
        alert.type = TYPE_CLAN;
        alert.priority = 50;
        
        alert.title = "Pending invite";
        alert.body = "\"" + clanName + "\" - Invited by " + invitedBy + " · " + memberCount.ToString() + " members";
        alert.actionLabel = "ACCEPT";
        alert.actionTarget = "accept_invite";
        alert.alertId = "invite_" + clanName;
        
        return alert;
    }
    
    // Factory: Welcome info (first time)
    static TXBAlert Welcome(int currentBalance)
    {
        TXBAlert alert = new TXBAlert();
        alert.type = TYPE_INFO;
        alert.priority = 10;
        
        alert.title = "WELCOME TO TRADERX BANKING";
        alert.body = "Balance: " + TXBFormat.Currency(currentBalance) + " roubles. Use the tabs to deposit cash, send transfers, or manage your vault.";
        alert.isDismissible = true;
        alert.alertId = "welcome";
        
        return alert;
    }
    
    // Factory: Clan promotion notification
    static TXBAlert ClanPromotion(int newRole)
    {
        TXBAlert alert = new TXBAlert();
        alert.type = TYPE_SUCCESS;
        alert.priority = 75;
        
        string roleName = TXBFormat.RoleBadge(newRole);
        
        alert.title = "Clan promotion";
        alert.body = "You have been promoted to " + roleName + ". You can now manage clan resources and member invites.";
        alert.alertId = "promotion_" + roleName;
        
        return alert;
    }
    
    // Factory: Generic info
    static TXBAlert Info(string titleText, string bodyText)
    {
        TXBAlert alert = new TXBAlert();
        alert.type = TYPE_INFO;
        alert.title = titleText;
        alert.body = bodyText;
        alert.alertId = "info_" + titleText;
        return alert;
    }
    
    // Get style class for panel (data-style binding in DZML)
    string GetStyleClass()
    {
        switch (type)
        {
            case TYPE_SUCCESS: return "TXSuccessBox";
            case TYPE_WARNING: return "TXWarningBox";
            case TYPE_DANGER: return "TXDangerBox";
            case TYPE_CLAN: return "TXInfoBox";
            default: return "TXInfoBox";
        }

        return "TXInfoBox";
    }
    
    // Get title color as ARGB hex for SetColor()
    // Usage: textWidget.SetColor(alert.GetTitleColorHex().ToInt());
    string GetTitleColorHex()
    {
        switch (type)
        {
            case TYPE_SUCCESS: return "FF9AB05A";  // Olive
            case TYPE_WARNING: return "FFD4A44C";  // Amber
            case TYPE_DANGER: return "FFC8664A";   // Rust
            case TYPE_CLAN: return "FF4A7AC7";     // Blue
            default: return "FF4A7AC7";            // Blue
        }

        return "FF4A7AC7";
    }
    
    // Check if alert has an action button
    bool HasAction()
    {
        return actionLabel != "" && actionTarget != "";
    }
    
    // Check if alert is expired
    bool IsExpired()
    {
        if (expiresAt <= 0) return false;
        
        int y, m, d, h, min, s;
        GetYearMonthDayUTC(y, m, d);
        GetHourMinuteSecondUTC(h, min, s);
        int now = y * 10000000000 + m * 100000000 + d * 1000000 + h * 10000 + min * 100 + s;
        
        return now > expiresAt;
    }
}
