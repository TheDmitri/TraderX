class TraderXPresetId
{
    private static ref map<string, int> s_PresetIdCounters = new map<string, int>();

    static void AssignPresetIdIfNot(TraderXPreset preset, string id = string.Empty)
    {
        if(preset.presetId == string.Empty)
        {
            if(id != string.Empty){
                preset.presetId = id;
            }
            else{
                GetTraderXLogger().LogDebug(string.Format("[TraderXPresetId] Generating ID for preset: %1 (product: %2)", preset.presetName, preset.productId));
                preset.presetId = GeneratePresetId(preset.productId, preset.presetName);
            }
        }
    }

    static bool IsValidPresetId(string filename)
    {
        string originalFilename = filename;
        
        // Remove .json extension if present
        if (filename.IndexOf(".json") != -1)
            filename = filename.Substring(0, filename.Length() - 5);
            
        // Check minimum length (at least "preset_x_001")
        if (filename.Length() < 11)
        {
            GetTraderXLogger().LogError(string.Format("[TraderXPreset] Invalid preset ID '%1': Too short (minimum length is 11 characters)", originalFilename));
            return false;
        }
            
        // Check if starts with "preset_"
        if (filename.SubstringUtf8(0, 7) != "preset_")
        {
            GetTraderXLogger().LogError(string.Format("[TraderXPreset] Invalid preset ID '%1': Must start with 'preset_'", originalFilename));
            return false;
        }
            
        // Find last underscore
        int lastUnderscorePos = filename.LastIndexOf("_");
        if (lastUnderscorePos == -1)
        {
            GetTraderXLogger().LogError(string.Format("[TraderXPreset] Invalid preset ID '%1': Missing underscore before number", originalFilename));
            return false;
        }
        if (lastUnderscorePos == 6)
        {
            GetTraderXLogger().LogError(string.Format("[TraderXPreset] Invalid preset ID '%1': Missing product ID part between 'preset_' and number", originalFilename));
            return false;
        }
        if (lastUnderscorePos > filename.Length() - 4)
        {
            GetTraderXLogger().LogError(string.Format("[TraderXPreset] Invalid preset ID '%1': Missing or incomplete number part after last underscore", originalFilename));
            return false;
        }
            
        // Extract and validate the number part
        string numberPart = filename.Substring(lastUnderscorePos + 1, filename.Length() - (lastUnderscorePos + 1));
        if (numberPart.Length() != 3)
        {
            GetTraderXLogger().LogError(string.Format("[TraderXPreset] Invalid preset ID '%1': Number part must be exactly 3 digits", originalFilename));
            return false;
        }
            
        // Check if all characters in number part are digits
        for (int i = 0; i < 3; i++)
        {
            string c = numberPart.Get(i);
            if (!(c >= "0" && c <= "9"))
            {
                GetTraderXLogger().LogError(string.Format("[TraderXPreset] Invalid preset ID '%1': Number part contains non-digit character '%2'", originalFilename, c));
                return false;
            }
        }
        
        // Convert to number and check range (001-999)
        int number = numberPart.ToInt();
        if (number < 1 || number > 999)
        {
            GetTraderXLogger().LogError(string.Format("[TraderXPreset] Invalid preset ID '%1': Number must be between 001 and 999", originalFilename));
            return false;
        }
            
        // Check if middle part (product ID + preset name) exists and contains valid characters
        string middlePart = filename.Substring(7, lastUnderscorePos - 7);
        if (middlePart == "")
        {
            GetTraderXLogger().LogError(string.Format("[TraderXPreset] Invalid preset ID '%1': Missing product ID part between 'preset_' and number", originalFilename));
            return false;
        }
            
        // Check if middle part contains only alphanumeric characters and underscores
        // Replace invalid characters with underscores
        string sanitizedMiddlePart = "";
        for (int j = 0; j < middlePart.Length(); j++)
        {
            string cc = middlePart.Get(j);
            if ((cc >= "a" && cc <= "z") || (cc >= "A" && cc <= "Z") || (cc >= "0" && cc <= "9") || cc == "_")
            {
                sanitizedMiddlePart += cc;
            }
            else
            {
                sanitizedMiddlePart += "_";
            }
        }
        
        // If we had to sanitize the filename, reconstruct it and log a warning
        if (sanitizedMiddlePart != middlePart)
        {
            string newFilename = "preset_" + sanitizedMiddlePart + "_" + numberPart;
            GetTraderXLogger().LogWarning(string.Format("[TraderXPreset] Preset ID '%1' contained invalid characters, sanitized to '%2'", originalFilename, newFilename));
            filename = newFilename;
        }
        
        return true;
    }

    static string GeneratePresetId(string productId, string presetName)
    {
        GetTraderXLogger().LogDebug(string.Format("[TraderXPresetId] Generating ID for preset: %1 (product: %2)", presetName, productId));
        
        // Convert productId and presetName to a valid filename part
        string baseFilename = productId + "_" + presetName;
        baseFilename.ToLower();
        baseFilename.Replace(" ", "_");
        baseFilename.Replace("-", "_");
        baseFilename.Replace(".", "_");
        
        // Get current counter for this base filename
        if (!s_PresetIdCounters.Contains(baseFilename))
        {
            s_PresetIdCounters.Set(baseFilename, 0);
        }
        
        int counter = s_PresetIdCounters.Get(baseFilename);
        string presetId = string.Empty;
        bool isValid = false;
        int loopCount = 0;
        
        while (!isValid && counter < 999)
        {
            counter++;
            loopCount++;
            presetId = string.Format("preset_%1_%2", baseFilename, counter.ToStringLen(3));
            
            isValid = IsValidPresetId(presetId);
            
            if (loopCount > 10)
            {
                GetTraderXLogger().LogWarning(string.Format("[TraderXPresetId] Loop %1 for %2, testing: %3", loopCount, baseFilename, presetId));
            }
            
            if (loopCount > 50)
            {
                GetTraderXLogger().LogError(string.Format("[TraderXPresetId] Breaking loop after %1 iterations to prevent infinite loop", loopCount));
                break;
            }
        }
        
        if (!isValid)
        {
            GetTraderXLogger().LogError(string.Format("[TraderXPreset] Failed to generate valid ID for '%1' after %2 attempts", baseFilename, loopCount));
            return string.Empty;
        }
        
        s_PresetIdCounters.Set(baseFilename, counter);
        return presetId;
    }

    static string ExtractProductIdFromPresetId(string presetId)
    {
        // Extract product ID from preset ID format: preset_[productId]_[presetName]_[counter]
        if (!presetId.Contains("preset_"))
            return string.Empty;
            
        // Remove "preset_" prefix
        string remaining = presetId.Substring(7, presetId.Length() - 7);
        
        // Find the last underscore (before counter)
        int lastUnderscorePos = remaining.LastIndexOf("_");
        if (lastUnderscorePos == -1)
            return string.Empty;
            
        // Extract everything before the last underscore
        string productAndName = remaining.Substring(0, lastUnderscorePos);
        
        // Find the first underscore (after product ID)
        int firstUnderscorePos = productAndName.IndexOf("_");
        if (firstUnderscorePos == -1)
            return productAndName; // No preset name, just product ID
            
        // Return just the product ID part
        return productAndName.Substring(0, firstUnderscorePos);
    }
}
