// TraderXBanking - Essential DZML Pipes
// Minimal set of banking-specific pipes for DZML bindings.
// Usage in layouts: {{ value | pipeName }}

// Currency - Format integer with thousands separator
// {{ 14165 | currency }} > "14,165"
class TXBCurrencyPipe extends DZPipe
{
    override string Transform(string value, array<string> args)
    {
        int amount = value.ToInt();
        
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
}

// Signed - Format with + or - sign
// {{ 500 | signed }} > "+500"
// {{ -200 | signed }} > "-200"
class TXBSignedPipe extends DZPipe
{
    override string Transform(string value, array<string> args)
    {
        int amount = value.ToInt();
        TXBCurrencyPipe currencyPipe = new TXBCurrencyPipe();
        
        if (amount >= 0)
            return "+" + currencyPipe.Transform(value, null);
        else
            return currencyPipe.Transform(value, null);
    }
}

// AccountId - Format Steam ID with dot separators
// {{ "76561198912345678" | accountId }} > "76561198-9123-4567"
class TXBAccountIdPipe extends DZPipe
{
    override string Transform(string value, array<string> args)
    {
        if (value.Length() < 17) return value;
        
        string part1 = value.Substring(0, 8);
        string part2 = value.Substring(8, 4);
        string part3 = value.Substring(12, 4);
        
        return part1 + "-" + part2 + "-" + part3;
    }
}

// TimeRemaining - Format hours as "Xd Xh"
// {{ 27 | timeRemaining }} > "1d 3h"
class TXBTimeRemainingPipe extends DZPipe
{
    override string Transform(string value, array<string> args)
    {
        int hours = value.ToInt();
        
        if (hours <= 0) return "now";
        
        int days = hours / 24;
        int hrs = hours % 24;
        
        if (days > 0 && hrs > 0)
            return days.ToString() + "d " + hrs.ToString() + "h";
        else if (days > 0)
            return days.ToString() + "d";
        else
            return hrs.ToString() + "h";
    }
}

// Date - Format YYYYMMDD timestamp
// {{ "20260417" | date }} > "2026-04-17"
class TXBDatePipe extends DZPipe
{
    override string Transform(string value, array<string> args)
    {
        return TXBTimeUtil.FormatDate(value.ToInt());
    }
}

// Register banking pipes (call during module init)
class TXBPipeRegistration
{
    static void RegisterAll()
    {
        PipeRegistry.RegisterPipe("currency", new TXBCurrencyPipe());
        PipeRegistry.RegisterPipe("signed", new TXBSignedPipe());
        PipeRegistry.RegisterPipe("accountId", new TXBAccountIdPipe());
        PipeRegistry.RegisterPipe("timeRemaining", new TXBTimeRemainingPipe());
        PipeRegistry.RegisterPipe("date", new TXBDatePipe());
        
        // Note: 'percent' pipe already exists in DayZular (Pipe.c)
    }
}
