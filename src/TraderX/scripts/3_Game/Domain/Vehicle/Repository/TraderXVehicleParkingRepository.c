class TraderXVehicleParkingRepository
{
    private static ref map<string, ref TraderXVehicleParkingCollection> s_AllParkingCollections = new map<string, ref TraderXVehicleParkingCollection>;
    private static ref TraderXVehicleParkingConfig s_ParkingConfig;
    
    static void LoadAllParkingCollections()
    {
        GetTraderXLogger().LogInfo("[TraderX] LoadAllParkingCollections - Starting");
        GetTraderXLogger().LogDebug("[TraderX] Expected parking directory: $profile:TraderX\\TraderXConfig\\TraderXVehicleParking\\");
        
        if (!FileExist(TRADERX_VEHICLE_PARKING_DIR)){
            GetTraderXLogger().LogInfo("[TraderX] Creating parking directory: " + TRADERX_VEHICLE_PARKING_DIR);
            MakeDirectory(TRADERX_VEHICLE_PARKING_DIR);
        }

        // Load parking configuration first
        LoadParkingConfig();

        s_AllParkingCollections.Clear();

        TraderXVehicleParkingCollection collection;
        string filename;
        FileAttr attr;
        string traderId;
        
        GetTraderXLogger().LogDebug("[TraderX] Starting file search in parking directory");
        GetTraderXLogger().LogDebug("[TraderX] Full parking directory path: " + TRADERX_VEHICLE_PARKING_DIR);
        FindFileHandle findHandle = FindFile(TRADERX_VEHICLE_PARKING_DIR + "parkings_*.json", filename, attr, FindFileFlags.ALL);
        
        GetTraderXLogger().LogDebug("[TraderX] First filename found: " + filename);
        
        if (findHandle == 0){
            GetTraderXLogger().LogError("[TraderX] ERROR: File handle is 0, no parking files found or directory error");
            GetTraderXLogger().LogError("[TraderX] Directory exists check completed");
            CloseFindFile(findHandle);
            return;
        }

        string fullFilePath;

        // Process first file if it matches parking pattern
        if (filename.Contains("parkings_") && filename.Contains(".json")) {
            fullFilePath = TRADERX_VEHICLE_PARKING_DIR + filename;
            GetTraderXLogger().LogDebug("[TraderX] Processing first file: " + fullFilePath);
            GetTraderXLogger().LogDebug("[TraderX] File exists check completed");
            
            collection = LoadParkingFile(fullFilePath);
            if(collection){
                traderId = ExtractTraderIdFromFilename(filename);
                GetTraderXLogger().LogDebug("SET : " + filename + " in s_AllParkingCollections with traderId " + traderId);
                s_AllParkingCollections.Set(traderId, collection);
            } else {
                GetTraderXLogger().LogError("[TraderX] Failed to load first parking file: " + fullFilePath);
            }
        } else {
            GetTraderXLogger().LogWarning("[TraderX] Skipping invalid parking filename: " + filename);
        }

        GetTraderXLogger().LogDebug("[TraderX] Starting to load remaining parking collections");
        while (FindNextFile(findHandle, filename, attr))
        {
            GetTraderXLogger().LogDebug("[TraderX] Found additional file: " + filename);
            
            if (!filename.Contains("parkings_") || !filename.Contains(".json")) {
                GetTraderXLogger().LogWarning("[TraderX] Skipping invalid parking filename: " + filename);
                continue;
            }
            
            fullFilePath = TRADERX_VEHICLE_PARKING_DIR + filename;
            GetTraderXLogger().LogDebug("[TraderX] Processing additional file: " + fullFilePath);
            GetTraderXLogger().LogDebug("[TraderX] File exists check completed");
            
            collection = LoadParkingFile(fullFilePath);
            if(collection){
                traderId = ExtractTraderIdFromFilename(filename);
                GetTraderXLogger().LogDebug("SET : " + filename + " in s_AllParkingCollections with traderId " + traderId);
                s_AllParkingCollections.Set(traderId, collection);
            } else {
                GetTraderXLogger().LogError("[TraderX] Failed to load parking from file: " + fullFilePath);
            }
        }

        CloseFindFile(findHandle);
        GetTraderXLogger().LogInfo(string.Format("[TraderX] LoadAllParkingCollections - Completed. Total parking files loaded: %1", s_AllParkingCollections.Count()));
    }
    
    private static TraderXVehicleParkingCollection LoadParkingFile(string filePath)
    {
        TraderXVehicleParkingCollection collection = new TraderXVehicleParkingCollection("");
        string errorMessage;
        if (JsonFileLoader<TraderXVehicleParkingCollection>.LoadFile(filePath, collection, errorMessage))
        {
            return collection;
        }
        GetTraderXLogger().LogError("LoadParkingFile: Failed to load " + filePath + " - " + errorMessage);
        return null;
    }
    
    private static string ExtractTraderIdFromFilename(string filename)
    {
        // Extract traderId from "parkings_[traderId].json"
        string traderId = filename;
        traderId.Replace("parkings_", "");
        traderId.Replace(".json", "");
        return traderId;
    }
    
    static TraderXVehicleParkingCollection GetParkingCollectionFromCache(string traderId)
    {
        if (s_AllParkingCollections.Contains(traderId))
        {
            return s_AllParkingCollections.Get(traderId);
        }
        return null;
    }
    
    static TraderXVehicleParkingCollection GetParkingCollection(string traderId)
    {
        GetTraderXLogger().LogDebug("[TraderX] GetParkingCollection called for trader: " + traderId);
        
        // First check cache
        TraderXVehicleParkingCollection cachedCollection = GetParkingCollectionFromCache(traderId);
        if (cachedCollection)
        {
            GetTraderXLogger().LogDebug("[TraderX] Found parking collection in cache for trader: " + traderId);
            return cachedCollection;
        }
        
        GetTraderXLogger().LogDebug("[TraderX] No cached collection found, attempting file load for trader: " + traderId);
        
        // Fallback to file loading
        string filePath = string.Format(TRADERX_VEHICLE_PARKING_FILE, traderId);
        GetTraderXLogger().LogDebug("[TraderX] Expected file path: " + filePath);
        GetTraderXLogger().LogDebug("[TraderX] File exists: " + FileExist(filePath));
        
        if (FileExist(filePath))
        {
            TraderXVehicleParkingCollection collection = new TraderXVehicleParkingCollection(traderId);
            string errorMessage;
            if (JsonFileLoader<TraderXVehicleParkingCollection>.LoadFile(filePath, collection, errorMessage))
            {
                GetTraderXLogger().LogDebug("[TraderX] Successfully loaded parking collection from file for trader: " + traderId);
                // Add to cache for future use
                s_AllParkingCollections.Set(traderId, collection);
                return collection;
            }
            GetTraderXLogger().LogError("GetParkingCollection: Failed to load " + filePath + " - " + errorMessage);
        } else {
            GetTraderXLogger().LogWarning("[TraderX] Parking file does not exist: " + filePath);
        }
        
        GetTraderXLogger().LogWarning("[TraderX] No parking collection found for trader: " + traderId);
        return null;
    }
    
    static void SaveParkingCollection(TraderXVehicleParkingCollection collection)
    {
        string filePath = string.Format(TRADERX_VEHICLE_PARKING_FILE, collection.traderId);
        string errorMessage;
        if (!JsonFileLoader<TraderXVehicleParkingCollection>.SaveFile(filePath, collection, errorMessage))
        {
            GetTraderXLogger().LogError("SaveParkingCollection: Failed to save " + filePath + " - " + errorMessage);
        }
        else
        {
            // Update cache
            s_AllParkingCollections.Set(collection.traderId, collection);
        }
    }
    
    static void CreateDefaultParkingCollection(string traderId)
    {
        TraderXVehicleParkingCollection collection = new TraderXVehicleParkingCollection(traderId);
        
        // Create some default parking positions (can be customized per trader)
        TraderXVehicleParkingPosition pos1 = new TraderXVehicleParkingPosition("0 0 5", "0 0 0", "3 2 6");
        TraderXVehicleParkingPosition pos2 = new TraderXVehicleParkingPosition("0 0 -5", "0 0 0", "3 2 6");
        TraderXVehicleParkingPosition pos3 = new TraderXVehicleParkingPosition("5 0 0", "0 90 0", "3 2 6");
        
        collection.positions.Insert(pos1);
        collection.positions.Insert(pos2);
        collection.positions.Insert(pos3);
        
        SaveParkingCollection(collection);
    }
    
    // ===== PARKING CONFIG METHODS =====
    
    static void LoadParkingConfig()
    {
        if (FileExist(TRADERX_VEHICLE_PARKING_CONFIG_FILE))
        {
            s_ParkingConfig = new TraderXVehicleParkingConfig();
            string errorMessage;
            if (JsonFileLoader<TraderXVehicleParkingConfig>.LoadFile(TRADERX_VEHICLE_PARKING_CONFIG_FILE, s_ParkingConfig, errorMessage))
            {
                GetTraderXLogger().LogInfo("Loaded vehicle parking config from file");
                return;
            }
            GetTraderXLogger().LogError("LoadParkingConfig: Failed to load " + TRADERX_VEHICLE_PARKING_CONFIG_FILE + " - " + errorMessage);
        }
        
        // Create default config if file doesn't exist or failed to load
        CreateDefaultParkingConfig();
    }
    
    static void CreateDefaultParkingConfig()
    {
        s_ParkingConfig = new TraderXVehicleParkingConfig();
        SaveParkingConfig();
        GetTraderXLogger().LogInfo("Created default vehicle parking config");
    }
    
    static void SaveParkingConfig()
    {
        if (!s_ParkingConfig)
        {
            return;
        }
        
        string errorMessage;
        if (!JsonFileLoader<TraderXVehicleParkingConfig>.SaveFile(TRADERX_VEHICLE_PARKING_CONFIG_FILE, s_ParkingConfig, errorMessage))
        {
            GetTraderXLogger().LogError("SaveParkingConfig: Failed to save " + TRADERX_VEHICLE_PARKING_CONFIG_FILE + " - " + errorMessage);
        }
        else
        {
            GetTraderXLogger().LogInfo("Saved vehicle parking config");
        }
    }
    
    static TraderXVehicleParkingConfig GetParkingConfig()
    {
        if (!s_ParkingConfig)
        {
            LoadParkingConfig();
        }
        return s_ParkingConfig;
    }
    
    static bool IsObjectWhitelistedForParking(string objectName)
    {
        TraderXVehicleParkingConfig config = GetParkingConfig();
        if (config)
        {
            return config.IsObjectWhitelisted(objectName);
        }
        
        // Fallback to basic terrain check if config not available
        return objectName.Contains("Land_") || objectName.Contains("Terrain");
    }
}