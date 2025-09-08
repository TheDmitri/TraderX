class TraderXCategoryId
{
    private static ref map<string, int> s_CategoryIdCounters = new map<string, int>();

    static bool IsValidUUID(string filename)
    {
        string originalFilename = filename;
        int i;
        string c;
        
        // Remove .json extension if present
        if (filename.IndexOf(".json") != -1)
            filename = filename.Substring(0, filename.Length() - 5);
            
        // Check minimum length (at least "cat_x_001")
        if (filename.Length() < 8)
        {
            GetTraderXLogger().LogError(string.Format("[TraderXCategory] Invalid category ID '%1': Too short (minimum length is 8 characters)", originalFilename));
            return false;
        }
            
        // Check if starts with "cat_"
        if (filename.SubstringUtf8(0, 4) != "cat_")
        {
            GetTraderXLogger().LogError(string.Format("[TraderXCategory] Invalid category ID '%1': Must start with 'cat_'", originalFilename));
            return false;
        }
            
        // Find last underscore
        int lastUnderscorePos = filename.LastIndexOf("_");
        if (lastUnderscorePos == -1)
        {
            GetTraderXLogger().LogError(string.Format("[TraderXCategory] Invalid category ID '%1': Missing underscore before number", originalFilename));
            return false;
        }
        if (lastUnderscorePos == 3)
        {
            GetTraderXLogger().LogError(string.Format("[TraderXCategory] Invalid category ID '%1': Missing filename part between 'cat_' and number", originalFilename));
            return false;
        }
        if (lastUnderscorePos > filename.Length() - 4)
        {
            GetTraderXLogger().LogError(string.Format("[TraderXCategory] Invalid category ID '%1': Missing or incomplete number part after last underscore", originalFilename));
            return false;
        }
            
        // Extract and validate the number part
        string numberPart = filename.Substring(lastUnderscorePos + 1, filename.Length() - (lastUnderscorePos + 1));
        if (numberPart.Length() != 3)
        {
            GetTraderXLogger().LogError(string.Format("[TraderXCategory] Invalid category ID '%1': Number part must be exactly 3 digits", originalFilename));
            return false;
        }
            
        // Check if all characters in number part are digits
        for (i = 0; i < 3; i++)
        {
            c = numberPart.Get(i);
            if (!(c >= "0" && c <= "9"))
            {
                GetTraderXLogger().LogError(string.Format("[TraderXCategory] Invalid category ID '%1': Number part contains non-digit character '%2'", originalFilename, c));
                return false;
            }
        }
        
        // Convert to number and check range (001-999)
        int number = numberPart.ToInt();
        if (number < 1 || number > 999)
        {
            GetTraderXLogger().LogError(string.Format("[TraderXCategory] Invalid category ID '%1': Number must be between 001 and 999", originalFilename));
            return false;
        }
            
        // Check middle part (between cat_ and _NNN) for valid characters
        string middlePart = filename.Substring(4, lastUnderscorePos - 4);
        if (middlePart == "")
        {
            GetTraderXLogger().LogError(string.Format("[TraderXCategory] Invalid category ID '%1': Missing filename part between 'cat_' and number", originalFilename));
            return false;
        }

        // Sanitize the middle part - replace invalid chars with underscores and handle consecutive underscores
        string sanitizedMiddlePart = "";
        bool lastWasUnderscore = false;
        
        for (i = 0; i < middlePart.Length(); i++)
        {
            c = middlePart.Get(i);
            // Check if current character is valid
            bool isValidChar = (c >= "a" && c <= "z") || (c >= "0" && c <= "9") || c == "_";
            
            // If character is invalid, use underscore
            string newChar;
            if(!isValidChar){
                newChar = "_";
            }else{
                newChar = c;
            }
            
            // Handle consecutive underscores
            if (newChar == "_" && lastWasUnderscore)
            {
                continue; // Skip this underscore to avoid consecutive ones
            }
            
            sanitizedMiddlePart += newChar;
            lastWasUnderscore = (newChar == "_");
        }
        
        // Remove leading and trailing underscores if present
        while (sanitizedMiddlePart != "" && sanitizedMiddlePart.Get(0) == "_")
        {
            sanitizedMiddlePart = sanitizedMiddlePart.Substring(1, sanitizedMiddlePart.Length() - 1);
        }
        while (sanitizedMiddlePart != "" && sanitizedMiddlePart.Get(sanitizedMiddlePart.Length() - 1) == "_")
        {
            sanitizedMiddlePart = sanitizedMiddlePart.Substring(0, sanitizedMiddlePart.Length() - 1);
        }
        
        // If empty after sanitization, use a default value
        if (sanitizedMiddlePart == "")
        {
            sanitizedMiddlePart = "category";
        }
        
        // If we had to sanitize the filename, reconstruct it and log a warning
        if (sanitizedMiddlePart != middlePart)
        {
            string newFilename = "cat_" + sanitizedMiddlePart + "_" + numberPart;
            GetTraderXLogger().LogWarning(string.Format("[TraderXCategory] Category ID '%1' contained invalid characters or format, sanitized to '%2'", originalFilename, newFilename + ".json"));
            filename = newFilename;
        }
        
        return true;
    }

    static string GenerateCategoryId(string catName)
    {
        // Convert category name to a valid filename part (replace invalid chars with underscore)
        catName.ToLower();
        string baseFilename = catName;
        baseFilename.Replace(" ", "_");
        baseFilename.Replace("-", "_");
        baseFilename.Replace(".", "_");
        
        // Get current counter for this base filename
        if (!s_CategoryIdCounters.Contains(baseFilename))
        {
            s_CategoryIdCounters.Set(baseFilename, 0);
        }
        
        int counter = s_CategoryIdCounters.Get(baseFilename);
        string catId = string.Empty;
        bool isValid = false;
        
        while (!isValid && counter < 999)
        {
            counter++;
            catId = string.Format("cat_%1_%2", baseFilename, counter.ToStringLen(3));
            isValid = IsValidUUID(catId);
        }
        
        if (!isValid)
        {
            GetTraderXLogger().LogError(string.Format("[TraderXCategory] Too many categories with base filename '%1' (max 999)", baseFilename));
            return string.Empty;
        }
        
        s_CategoryIdCounters.Set(baseFilename, counter);
        return catId;
    }

    static void AssignIdIfNot(TraderXCategory category, string id = string.Empty)
    {
        if(category.categoryId == string.Empty)
        {
            if(id != string.Empty){
                category.categoryId = id;
            }
            else
            {
                category.categoryId = GenerateCategoryId(category.categoryName);
            }
        }

        TraderXCategoryRepository.AddCategoryToCategories(category);
    }
}