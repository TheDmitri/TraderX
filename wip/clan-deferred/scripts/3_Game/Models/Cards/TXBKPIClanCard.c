// TraderXBanking - Clan KPI Card
// Displays clan info, member count, bank balance, and player contribution.
// Reusable in: HomeTab (KPI grid), ClanTab (header banner)

class TXBKPIClanCard : TXBKPICardBase
{
    // Raw data
    string clanId;
    string clanName;
    string clanTag;
    int memberCount;
    int maxMembers;
    int clanBankBalance;
    int vaultUsed;
    int vaultCapacity;
    int playerRole;  // TXBClanRole
    int contribution30d;
    
    void TXBKPIClanCard()
    {
        label = "CLAN";
        navTarget = "banking-clan-tab";
    }
    
    // Factory: Create from clan data and player info
    static TXBKPIClanCard FromClan(TXBClanData clan, int role, int contribution)
    {
        TXBKPIClanCard card = new TXBKPIClanCard();
        
        if (clan)
        {
            card.clanId = clan.clanId;
            card.clanName = clan.name;
            card.clanTag = clan.tag;
            card.memberCount = clan.members.Count();
            card.maxMembers = clan.maxMembers;
            card.clanBankBalance = clan.bankBalance;
            card.vaultUsed = clan.vaultUsed;
            card.vaultCapacity = clan.vaultCapacity;
            card.playerRole = role;
            card.contribution30d = contribution;
            
            // Format display values
            card.badge = TXBFormat.RoleBadge(role);
            card.valueText = TXBFormat.MemberCount(card.memberCount, card.maxMembers);
            card.subText = "Vault " + TXBFormat.Capacity(card.vaultUsed, card.vaultCapacity) + " · Bank " + TXBFormat.Currency(card.clanBankBalance);
            
            // No progress bar for clan card
            card.progressRatio = -1.0;
            
            // Footer
            card.footerLeft = TXBFormat.Contribution(contribution);
            
            // Update label with tag
            card.label = "CLAN · \"" + clan.tag + "\"";
        }
        else
        {
            card.isEmpty = true;
            card.emptyMessage = "No clan affiliation";
            card.valueText = "—";
            card.subText = "Join or create a clan";
            card.footerLeft = "—";
            card.badge = "—";
        }
        
        return card;
    }
    
    // Factory: Create empty state
    static TXBKPIClanCard Empty()
    {
        TXBKPIClanCard card = new TXBKPIClanCard();
        card.isEmpty = true;
        card.emptyMessage = "No clan";
        card.clanName = "No clan";
        card.valueText = "—";
        card.subText = "Join or create a clan";
        card.badge = "—";
        card.footerLeft = "—";
        return card;
    }
    
    // Check if player has Officer+ permissions
    bool CanManage()
    {
        return playerRole == TXBClanRole.OFFICER || playerRole == TXBClanRole.OWNER;
    }
    
    // Check if player is Owner
    bool IsOwner()
    {
        return playerRole == TXBClanRole.OWNER;
    }
    
    // Get role color
    override string GetBadgeColorClass()
    {
        switch (playerRole)
        {
            case TXBClanRole.OWNER: return "dz-tc-C8664A";   // Rust
            case TXBClanRole.OFFICER: return "dz-tc-D4A44C"; // Amber
            case TXBClanRole.MEMBER: return "dz-tc-9A9488";  // Ash
            default: return "dz-tc-6A6560";  // Dust
        }

        return "dz-tc-6A6560";  // Dust
    }
    
    // Get formatted header for ClanTab
    string GetHeaderTitle()
    {
        return clanName;
    }
    
    string GetHeaderSubtitle()
    {
        return "Vault " + TXBFormat.Capacity(vaultUsed, vaultCapacity) + " · Bank " + TXBFormat.Currency(clanBankBalance);
    }
    
    // Check if clan is near member capacity
    bool IsNearCapacity()
    {
        if (maxMembers <= 0) return false;
        return (memberCount / (maxMembers * 1.0)) > 0.8;
    }
}
