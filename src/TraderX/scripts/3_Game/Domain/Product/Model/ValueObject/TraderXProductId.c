class TraderXProductId
{
    private static ref map<string, int> s_ProductIdCounters = new map<string, int>();

    static void AssignUUIDIfNot(TraderXProduct product, string id = string.Empty)
    {
        if(product.productId == string.Empty)
        {
            if(id != string.Empty){
                product.productId = id;
            }
            else{
                GetTraderXLogger().LogDebug(string.Format("[TraderXProductId] Generating ID for: %1", product.className));
                product.productId = GenerateProductId(product.className);
            }
        }

        TraderXProductRepository.AddItemToItems(product);
    }

    static bool IsValidUUID(string filename)
    {
        string originalFilename = filename;
        
        // Remove .json extension if present
        if (filename.IndexOf(".json") != -1)
            filename = filename.Substring(0, filename.Length() - 5);
            
        // Check minimum length (at least "prod_x_001")
        if (filename.Length() < 9)
        {
            GetTraderXLogger().LogError(string.Format("[TraderXProduct] Invalid product ID '%1': Too short (minimum length is 9 characters)", originalFilename));
            return false;
        }
            
        // Check if starts with "prod_"
        if (filename.SubstringUtf8(0, 5) != "prod_")
        {
            GetTraderXLogger().LogError(string.Format("[TraderXProduct] Invalid product ID '%1': Must start with 'prod_'", originalFilename));
            return false;
        }
            
        // Find last underscore
        int lastUnderscorePos = filename.LastIndexOf("_");
        if (lastUnderscorePos == -1)
        {
            GetTraderXLogger().LogError(string.Format("[TraderXProduct] Invalid product ID '%1': Missing underscore before number", originalFilename));
            return false;
        }
        if (lastUnderscorePos == 4)
        {
            GetTraderXLogger().LogError(string.Format("[TraderXProduct] Invalid product ID '%1': Missing filename part between 'prod_' and number", originalFilename));
            return false;
        }
        if (lastUnderscorePos > filename.Length() - 4)
        {
            GetTraderXLogger().LogError(string.Format("[TraderXProduct] Invalid product ID '%1': Missing or incomplete number part after last underscore", originalFilename));
            return false;
        }
            
        // Extract and validate the number part
        string numberPart = filename.Substring(lastUnderscorePos + 1, filename.Length() - (lastUnderscorePos + 1));
        if (numberPart.Length() != 3)
        {
            GetTraderXLogger().LogError(string.Format("[TraderXProduct] Invalid product ID '%1': Number part must be exactly 3 digits", originalFilename));
            return false;
        }
            
        // Check if all characters in number part are digits
        for (int i = 0; i < 3; i++)
        {
            string c = numberPart.Get(i);
            if (!(c >= "0" && c <= "9"))
            {
                GetTraderXLogger().LogError(string.Format("[TraderXProduct] Invalid product ID '%1': Number part contains non-digit character '%2'", originalFilename, c));
                return false;
            }
        }
        
        // Convert to number and check range (001-999)
        int number = numberPart.ToInt();
        if (number < 1 || number > 999)
        {
            GetTraderXLogger().LogError(string.Format("[TraderXProduct] Invalid product ID '%1': Number must be between 001 and 999", originalFilename));
            return false;
        }
            
        // Check if middle part (filename) exists and contains valid characters
        string middlePart = filename.Substring(5, lastUnderscorePos - 5);
        if (middlePart == "")
        {
            GetTraderXLogger().LogError(string.Format("[TraderXProduct] Invalid product ID '%1': Missing filename part between 'prod_' and number", originalFilename));
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
            string newFilename = "prod_" + sanitizedMiddlePart + "_" + numberPart;
            GetTraderXLogger().LogWarning(string.Format("[TraderXProduct] Product ID '%1' contained invalid characters, sanitized to '%2'", originalFilename, newFilename + ".json"));
            filename = newFilename;
        }
        
        return true;
    }

    static string GeneratePlayerItemId(string classname, int networkIdHigh, int networkIdLow)
    {
        // Generate unique ID for player items using network IDs
        // Format: player_classname_high_low (no validation needed)
        classname.ToLower();
        return string.Format("player_%1_%2_%3", classname, networkIdHigh, networkIdLow);
    }

    static string GenerateProductId(string classname)
    {
        GetTraderXLogger().LogDebug(string.Format("[TraderXProductId] Generating ID for: %1", classname));
        
        // Convert className to a valid filename part (replace invalid chars with underscore)
        classname.ToLower();
        string baseFilename = classname;
        baseFilename.Replace(" ", "_");
        baseFilename.Replace("-", "_");
        baseFilename.Replace(".", "_");
        
        // Get current counter for this base filename
        if (!s_ProductIdCounters.Contains(baseFilename))
        {
            s_ProductIdCounters.Set(baseFilename, 0);
        }
        
        int counter = s_ProductIdCounters.Get(baseFilename);
        string prodId = string.Empty;
        bool isValid = false;
        int loopCount = 0;
        
        while (!isValid && counter < 999)
        {
            counter++;
            loopCount++;
            prodId = string.Format("prod_%1_%2", baseFilename, counter.ToStringLen(3));
            
            isValid = IsValidUUID(prodId);
            
            if (loopCount > 10)
            {
                GetTraderXLogger().LogWarning(string.Format("[TraderXProductId] Loop %1 for %2, testing: %3", loopCount, baseFilename, prodId));
            }
            
            if (loopCount > 50)
            {
                GetTraderXLogger().LogError(string.Format("[TraderXProductId] Breaking loop after %1 iterations to prevent infinite loop", loopCount));
                break;
            }
        }
        
        if (!isValid)
        {
            GetTraderXLogger().LogError(string.Format("[TraderXProduct] Failed to generate valid ID for '%1' after %2 attempts", baseFilename, loopCount));
            return string.Empty;
        }
        
        s_ProductIdCounters.Set(baseFilename, counter);
        return prodId;
    }
}