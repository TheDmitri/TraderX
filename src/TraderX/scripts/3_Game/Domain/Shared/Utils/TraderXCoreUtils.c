/**
 * TraderXCoreUtils
 * Core utility functions for TraderX mod
 * 
 * This class provides common utility functions used across the TraderX mod.
 * It includes methods for formatting, display name resolution, and UI color handling.
 */
class TraderXCoreUtils
{
    /**
     * Prints the binary representation of an integer value
     * @param value The integer to convert to binary
     * @param output The output string to append the binary representation to
     */
    static void PrintIntegerBits(int value, out string output)
    {
        for(int i = 31; i>= 0; i--)
        {
            int bit = (value >> i) & 1;
            output += bit.ToString() + "  ";
            if(i%4 == 0)
                output += "| ";
        }
    }

    /**
     * Formats seconds into hours:minutes:seconds format
     * @param seconds Total seconds to format
     * @return Formatted time string in HH:MM:SS format
     */
    static string GetFormatedHoursFromSeconds(int seconds)
    {
        int hours = seconds / 3600;
        int minutes = (seconds - hours * 3600) / 60;
        int seconds2 = seconds - hours * 3600 - minutes * 60;
        
        // Add zero padding for single digits
        string minutesStr = Ternary<string>.If(minutes < 10, "0" + minutes.ToString(), minutes.ToString());
        string secondsStr = Ternary<string>.If(seconds2 < 10, "0" + seconds2.ToString(), seconds2.ToString());
        
        return hours.ToString() + ":" + minutesStr + ":" + secondsStr;
    }

    static string GenerateShortDateString()
    {
        int year, month, day;
        GetYearMonthDay(year, month, day);
        return "" + year + "_" + month + "_" + day;
    }

    static string GenerateShortTimeString()
    {
        int hour, minute, second;
        GetHourMinuteSecond(hour, minute, second);
        return "" + hour + "_" + minute + "_" + second;
    }

    static string GenerateFullTimestamp()
    {
        string dateStr = GenerateShortDateString();
        string timeStr = GenerateShortTimeString();

        return dateStr + "-" + timeStr;
    }

    /**
     * Removes "$UNT$" from a string
     * @param input String to process
     * @return String with "$UNT$" removed
     */
    static string TrimUnt(string input)
    {
        input.Replace("$UNT$", "");
        return input;
    }

    /**
     * Gets the display name for an item class
     * @param className Class name to look up
     * @return Display name for the class
     */
    static string GetDisplayName(string className)
    {
        if (!className || className == "")
            return "";
            
        string displayName = className;

        if (GetGame().ConfigGetText(CFG_VEHICLESPATH + " " + className + " displayName", displayName))
        {
            displayName = TrimUnt(displayName);
        }

        if (GetGame().ConfigGetText(CFG_WEAPONSPATH + " " + className + " displayName", displayName))
        {
            displayName = TrimUnt(displayName);
        }

        if (GetGame().ConfigGetText(CFG_MAGAZINESPATH + " " + className + " displayName", displayName))
        {
            displayName = TrimUnt(displayName);
        }

        return displayName;
    }

    /**
     * Gets color values for a specific health level
     * @param healthLevel Health level constant from GameConstants
     * @param cA Alpha channel value (output)
     * @param cR Red channel value (output)
     * @param cG Green channel value (output)
     * @param cB Blue channel value (output)
     */
    static void GetHealthFromLevel(int healthLevel, out int cA, out int cR, out int cG, out int cB)
    {
        switch(healthLevel)
        {
            case GameConstants.STATE_PRISTINE:
                cA=255;
                cR=40;
                cG=159;
                cB=70;
                return;

            case GameConstants.STATE_BADLY_DAMAGED:
                cA=255;
                cR=161;
                cG=87;
                cB=13;
                return;

            case GameConstants.STATE_DAMAGED:
                cA=255;
                cR=212;
                cG=149;
                cB=86;
                return;

            case GameConstants.STATE_WORN:
                cA=255;
                cR=129;
                cG=207;
                cB=149;
                return;

            case GameConstants.STATE_RUINED:
                cA=255;
                cR=159;
                cG=40;
                cB=40;
                return;

            default:
                cA=255;
                cR=255;
                cG=255;
                cB=255;
                return;
        }
    }
}
