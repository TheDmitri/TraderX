// TraderXBanking - Format Utilities
// Centralized formatting helpers for currency, dates, and display values.
// Used by all ViewModel objects to ensure consistent formatting across the UI.

class TXBFormat
{
    // Currency formatting with thousands separator
    static string Currency(int amount)
    {
        if (amount == 0) return "0";
        
        bool negative = amount < 0;
        int absAmount = Math.AbsInt(amount);
        
        string result = "";
        int count = 0;
        
        while (absAmount > 0)
        {
            if (count > 0 && count % 3 == 0)
                result = "," + result;
            
            int digit = absAmount % 10;
            result = digit.ToString() + result;
            absAmount = absAmount / 10;
            count++;
        }
        
        if (negative)
            result = "-" + result;
            
        return result;
    }
    
    // Delta with sign (+X or -X)
    static string Delta(int amount)
    {
        if (amount >= 0)
            return "+" + Currency(amount);
        else
            return Currency(amount);
    }
    
    // Percentage for progress bars (0.0 to 1.0)
    static float Ratio(int current, int maximum)
    {
        if (maximum <= 0) return 0.0;
        return Math.Clamp(current / (maximum * 1.0), 0.0, 1.0);
    }
    
    // Format percentage string (e.g., "28.3%")
    static string Percentage(float ratio)
    {
        int pct = Math.Round(ratio * 100);
        return pct.ToString() + "%";
    }
    
    // Format Steam ID with separators (76561198·A9F2·1847)
    static string AccountId(string rawUid)
    {
        if (rawUid.Length() < 17) return rawUid;
        
        string part1 = rawUid.Substring(0, 8);
        string part2 = rawUid.Substring(8, 4);
        string part3 = rawUid.Substring(12, 4);
        
        return part1 + "·" + part2 + "·" + part3;
    }
    
    // Format timestamp YYYYMMDDhhmmss to readable date
    static string DateTime(int utcStamp)
    {
        int year = utcStamp / 10000000000;
        int month = (utcStamp / 100000000) % 100;
        int day = (utcStamp / 1000000) % 100;
        int hour = (utcStamp / 10000) % 100;
        int minute = (utcStamp / 100) % 100;
        
        string y = year.ToString();
        string m = month.ToString();
        string d = day.ToString();
        string h = hour.ToString();
        string min = minute.ToString();
        
        if (month < 10) m = "0" + m;
        if (day < 10) d = "0" + d;
        if (hour < 10) h = "0" + h;
        if (minute < 10) min = "0" + min;
        
        return y + "-" + m + "-" + d + " " + h + ":" + min;
    }
    
    // Format date only (YYYY-MM-DD)
    static string Date(int utcStamp)
    {
        int year = utcStamp / 10000000000;
        int month = (utcStamp / 100000000) % 100;
        int day = (utcStamp / 1000000) % 100;
        
        string y = year.ToString();
        string m = month.ToString();
        string d = day.ToString();
        
        if (month < 10) m = "0" + m;
        if (day < 10) d = "0" + d;
        
        return y + "-" + m + "-" + d;
    }
    
    // Format relative time (e.g., "expires in 2d", "3h 12m")
    static string TimeRemaining(int hoursRemaining)
    {
        if (hoursRemaining <= 0) return "now";
        
        int days = hoursRemaining / 24;
        int hours = hoursRemaining % 24;
        
        if (days > 0 && hours > 0)
            return days.ToString() + "d " + hours.ToString() + "h";
        else if (days > 0)
            return days.ToString() + "d";
        else
            return hours.ToString() + "h";
    }
    
    // Format session ID with prefix
    static string SessionId(int sessionNum)
    {
        return "#" + sessionNum.ToString();
    }
    
    // Format vault box number
    static string VaultBox(int boxNum)
    {
        return "Box #" + boxNum.ToString();
    }
    
    // Format capacity (e.g., "42 / 100")
    static string Capacity(int used, int total)
    {
        return used.ToString() + " / " + total.ToString();
    }
    
    // Format fee per day
    static string DailyFee(int feePerSlot, int slotsUsed)
    {
        int total = feePerSlot * slotsUsed;
        return "-" + Currency(total) + " /day";
    }
    
    // Format role badge text
    static string RoleBadge(int role)
    {
        switch (role)
        {
            case TXBClanRole.OWNER: return "Owner";
            case TXBClanRole.OFFICER: return "Officer";
            case TXBClanRole.MEMBER: return "Member";
            default: return "—";
        }

        return "—";
    }
    
    // Format member count
    static string MemberCount(int current, int max)
    {
        return current.ToString() + " members";
    }
    
    // Format contribution
    static string Contribution(int amount)
    {
        return "+" + Currency(amount) + " contrib. 30d";
    }
    
    // Format transaction count
    static string TransactionCount(int transfers, int total)
    {
        return transfers.ToString() + " transfers · " + total.ToString() + " tx";
    }
}
