// TraderXBanking - Hero Card Data
// Welcome section with player identity and session info.
// Used by Home Tab hero section.

class TXBHeroCard
{
    // Player identity
    string playerName;
    string rawUid;
    
    // Formatted display (computed)
    string formattedWelcome;
    string formattedAccountId;
    string lastVisitDisplay;
    string sessionDisplay;
    
    // Raw values
    int sessionId;
    int lastVisitUtc;
    
    void TXBHeroCard()
    {
        playerName = "—";
        rawUid = "—";
        formattedWelcome = "Welcome back, —";
        formattedAccountId = "—";
        lastVisitDisplay = "Session opened";
        sessionDisplay = "#—";
        sessionId = 1;
        lastVisitUtc = 0;
    }
    
    // Factory: Create from player identity
    static TXBHeroCard FromIdentity(PlayerIdentity identity)
    {
        TXBHeroCard card = new TXBHeroCard();
        
        if (identity)
        {
            card.playerName = identity.GetName();
            card.rawUid = identity.GetPlainId();
            card.formattedWelcome = "Welcome back, " + card.playerName;
            card.formattedAccountId = TXBFormat.AccountId(card.rawUid);
        }
        
        return card;
    }
    
    // Factory: Create with session info
    static TXBHeroCard FromIdentityAndSession(PlayerIdentity identity, int sessionNum)
    {
        TXBHeroCard card = FromIdentity(identity);
        card.sessionId = sessionNum;
        card.sessionDisplay = TXBFormat.SessionId(sessionNum);
        return card;
    }
    
    // Update last visit timestamp
    void UpdateLastVisit(int utcStamp)
    {
        lastVisitUtc = utcStamp;
        lastVisitDisplay = "Last visit · " + TXBFormat.Date(utcStamp);
    }
}
