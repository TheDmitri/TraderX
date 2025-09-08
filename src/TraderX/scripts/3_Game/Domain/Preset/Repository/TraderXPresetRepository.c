class TraderXPresetRepository
{
    static map<string, ref TraderXPresets> LoadAllPresets()
    {
        map<string, ref TraderXPresets> allPresets = new map<string, ref TraderXPresets>;
        GetTraderXLogger().LogInfo("[TraderX] LoadAllPresets - Starting");
        
        if (!FileExist(TRADERX_PRESETS)){
            MakeDirectory(TRADERX_PRESETS);
        }

        allPresets.Clear();

        TraderXPresets presets;
        string filename;
        FileAttr attr;
        string productId;
        
        GetTraderXLogger().LogDebug("[TraderX] Starting file search in: " + TRADERX_PRESETS);
        FindFileHandle findHandle = FindFile(TRADERX_PRESETS + "*.json", filename, attr, FindFileFlags.ALL);
        
        GetTraderXLogger().LogDebug("[TraderX] First filename found: " + filename);
        
        if (findHandle == 0){
            GetTraderXLogger().LogError("[TraderX] ERROR: File handle is 0, no preset files found or directory error");
            CloseFindFile(findHandle);
            return allPresets;
        }

        // Process first file if it matches preset pattern
        if (filename.Contains("presets_") && filename.Contains(".json")) {
            presets = LoadPresetFile(TRADERX_PRESETS + filename);
            if(presets){
                productId = ExtractProductIdFromFilename(filename);
                GetTraderXLogger().LogDebug("SET : " + filename + " in allPresets with productId " + productId);
                allPresets.Set(productId, presets);
            } else {
                GetTraderXLogger().LogError("[TraderX] Failed to load first preset file");
            }
        } else {
            GetTraderXLogger().LogWarning("[TraderX] Skipping invalid preset filename: " + filename);
        }

        GetTraderXLogger().LogDebug("[TraderX] Starting to load remaining presets");
        while (FindNextFile(findHandle, filename, attr))
        {
            if (!filename.Contains("presets_") || !filename.Contains(".json")) {
                GetTraderXLogger().LogWarning("[TraderX] Skipping invalid preset filename: " + filename);
                continue;
            }
            
            presets = LoadPresetFile(TRADERX_PRESETS + filename);
            if(presets){
                GetTraderXLogger().LogDebug("SET : " + filename + " in allPresets with productId " + presets.productId);
                allPresets.Set(presets.productId, presets);
            } else {
                GetTraderXLogger().LogError("[TraderX] Failed to load preset from file: " + filename);
            }
        }

        CloseFindFile(findHandle);
        GetTraderXLogger().LogInfo(string.Format("[TraderX] LoadAllPresets - Completed. Total preset files loaded: %1", allPresets.Count()));
        return allPresets;
    }
    
    private static TraderXPresets LoadPresetFile(string filePath)
    {
        TraderXPresets presets = new TraderXPresets();
        string errorMessage;
        if (JsonFileLoader<TraderXPresets>.LoadFile(filePath, presets, errorMessage))
        {
            return presets;
        }
        GetTraderXLogger().LogError("LoadPresetFile: Failed to load " + filePath + " - " + errorMessage);
        return null;
    }
    
    private static string ExtractProductIdFromFilename(string filename)
    {
        // Extract productId from "presets_[productId].json"
        string productId = filename;
        productId.Replace("presets_", "");
        productId.Replace(".json", "");
        return productId;
    }
    
    static void SavePresets(TraderXPresets presets)
    {
        string filePath = string.Format(TRADERX_PRESETS_FILE,presets.productId);
        string errorMessage;
        if (!JsonFileLoader<TraderXPresets>.SaveFile(filePath, presets, errorMessage))
        {
            GetTraderXLogger().LogError("SavePresets: Failed to save " + filePath + " - " + errorMessage);
        }
    }
    
    private static void CreateDefaultPresetConfig(string productId)
    {
        TraderXPresets presets = new TraderXPresets();
        presets.productId = productId;
        
        // Create basic server preset
        array<string> basicAttachments = new array<string>;
        TraderXPreset basicPreset = TraderXPreset.CreateTraderXPreset("Standard", productId, basicAttachments);
        presets.presets.Insert(basicPreset);
        presets.defaultPresetId = basicPreset.presetId;
        
        SavePresets(presets);
    }
    
    // Method to create server presets for vehicles
    static void CreateVehiclePresets(string vehicleClassName)
    {
        TraderXPresets presets = new TraderXPresets();
        presets.productId = vehicleClassName;
        
        // Standard preset
        array<string> standardParts = new array<string>;
        standardParts.Insert("CarBattery");
        standardParts.Insert("SparkPlug");
        standardParts.Insert("CarRadiator");
        
        TraderXPreset standardPreset = TraderXPreset.CreateTraderXPreset("Standard Package", vehicleClassName, standardParts);
        presets.presets.Insert(standardPreset);
        presets.defaultPresetId = standardPreset.presetId;
        
        // Military preset (example for vehicles)
        if (vehicleClassName.Contains("Offroad"))
        {
            array<string> militaryParts = new array<string>;
            militaryParts.Insert("CarBattery");
            militaryParts.Insert("SparkPlug");
            militaryParts.Insert("CarRadiator");
            militaryParts.Insert("CamoNet");
            
            TraderXPreset militaryPreset = TraderXPreset.CreateTraderXPreset("Military Package", vehicleClassName, militaryParts);
            presets.presets.Insert(militaryPreset);
        }
        
        SavePresets(presets);
    }
    
    // Method to create server presets for weapons
    static void CreateWeaponPresets(string weaponClassName)
    {
        TraderXPresets presets = new TraderXPresets();
        presets.productId = weaponClassName;
        
        // Basic preset
        array<string> basicAttachments = new array<string>;
        TraderXPreset basicPreset = TraderXPreset.CreateTraderXPreset("Basic", weaponClassName, basicAttachments);
        presets.presets.Insert(basicPreset);
        presets.defaultPresetId = basicPreset.presetId;
        
        // Tactical preset (example for AK weapons)
        if (weaponClassName.Contains("AK"))
        {
            array<string> tacticalAttachments = new array<string>;
            tacticalAttachments.Insert("AK_PlasticBttstck");
            tacticalAttachments.Insert("AK_RailHndgrd");
            tacticalAttachments.Insert("M4_Suppressor");
            tacticalAttachments.Insert("ACOGOptic");
            
            TraderXPreset tacticalPreset = TraderXPreset.CreateTraderXPreset("Tactical Package", weaponClassName, tacticalAttachments);
            presets.presets.Insert(tacticalPreset);
        }
        
        SavePresets(presets);
    }
}
