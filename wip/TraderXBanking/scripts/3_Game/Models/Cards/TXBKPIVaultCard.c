// TraderXBanking - Vault KPI Card
// Displays vault capacity, fees, and next collection time.
// Reusable in: HomeTab (KPI grid), VaultTab (header)

class TXBKPIVaultCard : TXBKPICardBase
{
    // Raw data
    int slotsUsed;
    int capacity;
    int feePerSlot;
    int hoursUntilFee;
    int boxNumber;
    string ownerType;  // "player" or "clan"
    string ownerName;  // For clan vault: clan name
    
    void TXBKPIVaultCard()
    {
        label = "PERSONAL VAULT";
        navTarget = "banking-vault-tab";
    }
    
    // Factory: Create from vault data
    static TXBKPIVaultCard FromVault(TXBVaultData vault)
    {
        TXBKPIVaultCard card = new TXBKPIVaultCard();
        
        if (vault && vault.capacity > 0)
        {
            card.slotsUsed = vault.slotsUsed;
            card.capacity = vault.capacity;
            card.feePerSlot = vault.feePerSlotPerDay;
            card.boxNumber = 47;  // Could be derived from vault ID
            card.ownerType = vault.ownerType;
            
            // Format display values
            card.badge = TXBFormat.VaultBox(card.boxNumber);
            card.valueText = TXBFormat.Capacity(card.slotsUsed, card.capacity);
            card.subText = "slots · next fee " + TXBFormat.TimeRemaining(card.hoursUntilFee);
            
            // Progress bar
            card.progressRatio = TXBFormat.Ratio(card.slotsUsed, card.capacity);
            
            // Footer
            int dailyTotal = card.feePerSlot * card.slotsUsed;
            card.footerLeft = "-" + TXBFormat.Currency(dailyTotal) + " /day";
            
            // For clan vault, update label
            if (card.ownerType == "clan")
            {
                card.label = "CLAN VAULT";
                card.ownerName = vault.ownerId;  // Simplified
            }
        }
        else
        {
            card.isEmpty = true;
            card.emptyMessage = "No vault configured";
            card.valueText = "— / —";
            card.subText = "Rent a vault box from a banker";
            card.footerLeft = "—";
            card.badge = "";
        }
        
        return card;
    }
    
    // Factory: Create from vault with custom box number
    static TXBKPIVaultCard FromVaultWithBox(TXBVaultData vault, int boxNum)
    {
        TXBKPIVaultCard card = FromVault(vault);
        card.boxNumber = boxNum;
        if (!card.isEmpty)
            card.badge = TXBFormat.VaultBox(boxNum);
        return card;
    }
    
    // Factory: Create for clan vault specifically
    static TXBKPIVaultCard FromClanVault(TXBVaultData vault, string clanName)
    {
        TXBKPIVaultCard card = FromVault(vault);
        card.label = "CLAN VAULT";
        card.ownerType = "clan";
        card.ownerName = clanName;
        card.badge = clanName;
        return card;
    }
    
    // Check if vault fee is due soon (within 24h)
    bool IsFeeDueSoon()
    {
        return hoursUntilFee >= 0 && hoursUntilFee <= 24;
    }
    
    // Check if vault is frozen (fees unpaid)
    bool IsFrozen()
    {
        return hoursUntilFee < 0;
    }
    
    // Get warning message if applicable
    string GetWarningMessage()
    {
        if (IsFrozen())
            return "Vault frozen - fees unpaid";
        if (IsFeeDueSoon())
            return "Fee due in " + TXBFormat.TimeRemaining(hoursUntilFee);
        return "";
    }
    
    // Get formatted fee display for alerts
    string GetFeeAlertText()
    {
        int totalFee = feePerSlot * slotsUsed;
        return TXBFormat.Currency(totalFee) + " will be auto-debited from your bank balance.";
    }
}
