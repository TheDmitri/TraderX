class TraderXVehicleParkingService
{
    static ref TraderXVehicleParkingService m_instance;

    static ref ScriptInvoker Event_OnVehicleParkingDataReceived = new ScriptInvoker();
    
    ref map<string, ref array<EntityAI>> m_occupiedPositions; // traderId -> array of vehicles in parking spots
    
    void TraderXVehicleParkingService()
    {
        m_occupiedPositions = new map<string, ref array<EntityAI>>();
        
        // Server-side initialization
        if (GetGame().IsServer())
        {
            LoadAllParkingCollections();
        }
    }
    
    static TraderXVehicleParkingService GetInstance()
    {
        if (!m_instance)
        {
            m_instance = new TraderXVehicleParkingService();
        }
        return m_instance;
    }

    void RegisterRPCs()
    {
        if(GetGame().IsClient())
        {
            GetRPCManager().AddRPC("TraderX", "GetVehicleParkingDataResponse", this, SingeplayerExecutionType.Client);
        }
        else
        {
            GetRPCManager().AddRPC("TraderX", "GetVehicleParkingDataRequest", this, SingeplayerExecutionType.Server);
        }
    }
    
    void LoadAllParkingCollections()
    {
        TraderXVehicleParkingRepository.LoadAllParkingCollections();
    }
    
    // Find an available parking position for a trader
    TraderXVehicleParkingPosition FindAvailableParkingPosition(string traderId)
    {
        TraderXVehicleParkingCollection collection = TraderXVehicleParkingRepository.GetParkingCollection(traderId);
        if (!collection)
        {
            GetTraderXLogger().LogWarning("No parking collection found for trader: " + traderId);
            return null;
        }
        
        // Get currently occupied positions for this trader
        array<EntityAI> occupiedVehicles = GetOccupiedVehicles(traderId);
        
        for (int i = 0; i < collection.positions.Count(); i++)
        {
            TraderXVehicleParkingPosition position = collection.positions[i];
            
            if (IsParkingPositionAvailable(position, occupiedVehicles))
            {
                return position;
            }
        }
        
        GetTraderXLogger().LogWarning("No available parking positions for trader: " + traderId);
        return null;
    }
    
    // Check if a parking position is available using collision detection
    bool IsParkingPositionAvailable(TraderXVehicleParkingPosition position, array<EntityAI> occupiedVehicles)
    {
        vector positionVec = position.position;
        vector rotationVec = position.rotation;
        vector sizeVec = position.size;
        
        return IsParkingAvailable(positionVec, rotationVec, sizeVec);
    }
    
    // Enhanced parking availability check using game collision detection
    static bool IsParkingAvailable(vector carpos, vector carori, vector size)
    {
        array<Object> excluded_objects = new array<Object>;
        array<Object> nearby_objects = new array<Object>;

        GetGame().IsBoxColliding(carpos, carori, size, excluded_objects, nearby_objects);
        if (nearby_objects.Count() > 0)
        {
            GetTraderXLogger().LogDebug("IsParkingAvailable collision detected with: " + nearby_objects.Get(0).GetType());
            
            for(int i = 0; i < nearby_objects.Count(); i++)
            {
                string objectName = nearby_objects.Get(i).GetType();
                
                // Check against whitelisted objects that don't block parking
                if (!IsObjectWhitelistedForParking(objectName))
                {
                    GetTraderXLogger().LogDebug("Parking blocked by object: " + objectName);
                    return false;
                }
            }
        }
        return true;
    }
    
    // Check if object type is whitelisted for parking (doesn't block vehicles)
    private static bool IsObjectWhitelistedForParking(string objectName)
    {
        return TraderXVehicleParkingRepository.IsObjectWhitelistedForParking(objectName);
    }
    
    // Reserve a parking position by spawning/moving a vehicle there
    bool ReserveParkingPosition(string traderId, TraderXVehicleParkingPosition position, EntityAI vehicle)
    {
        if (!position || !vehicle)
        {
            return false;
        }
        
        // Move vehicle to parking position with roll trick to force physics acknowledgment
        vector parkingPos = position.position;
        vector parkingRot = position.rotation;
        
        vehicle.SetPosition(parkingPos);
        vehicle.SetOrientation(parkingRot);
        vector roll = vehicle.GetOrientation();
        roll[2] = roll[2] - 1;
        vehicle.SetOrientation(roll);
        roll[2] = roll[2] + 1;
        vehicle.SetOrientation(roll);
        
        // Track occupied position
        if (!m_occupiedPositions.Contains(traderId))
        {
            m_occupiedPositions.Set(traderId, new array<EntityAI>());
        }
        
        array<EntityAI> occupiedVehicles = m_occupiedPositions.Get(traderId);
        if (occupiedVehicles.Find(vehicle) == -1)
        {
            occupiedVehicles.Insert(vehicle);
        }
        
        GetTraderXLogger().LogInfo("Vehicle reserved parking position for trader: " + traderId + " at position: " + parkingPos.ToString());
        return true;
    }
    
    // Release a parking position when vehicle is sold/removed
    void ReleaseParkingPosition(string traderId, EntityAI vehicle)
    {
        if (!m_occupiedPositions.Contains(traderId))
        {
            return;
        }
        
        array<EntityAI> occupiedVehicles = m_occupiedPositions.Get(traderId);
        int index = occupiedVehicles.Find(vehicle);
        if (index != -1)
        {
            occupiedVehicles.Remove(index);
            GetTraderXLogger().LogInfo("Vehicle released parking position for trader: " + traderId);
        }
    }
    
    // Get all vehicles currently occupying parking positions for a trader
    array<EntityAI> GetOccupiedVehicles(string traderId)
    {
        if (m_occupiedPositions.Contains(traderId))
        {
            return m_occupiedPositions.Get(traderId);
        }
        
        return new array<EntityAI>();
    }
    
    // Get total parking capacity for a trader
    int GetParkingCapacity(string traderId)
    {
        TraderXVehicleParkingCollection collection = TraderXVehicleParkingRepository.GetParkingCollection(traderId);
        if (collection)
        {
            return collection.positions.Count();
        }
        return 0;
    }
    
    // Get available parking count for a trader
    int GetAvailableParkingCount(string traderId)
    {
        int totalCapacity = GetParkingCapacity(traderId);
        int occupiedCount = GetOccupiedVehicles(traderId).Count();
        return totalCapacity - occupiedCount;
    }
    
    // Check if trader has any available parking spots
    bool HasAvailableParking(string traderId)
    {
        return GetAvailableParkingCount(traderId) > 0;
    }
    
    // Create default parking collection for a trader if it doesn't exist
    void EnsureParkingCollectionExists(string traderId)
    {
        TraderXVehicleParkingCollection collection = TraderXVehicleParkingRepository.GetParkingCollection(traderId);
        if (!collection)
        {
            TraderXVehicleParkingRepository.CreateDefaultParkingCollection(traderId);
            GetTraderXLogger().LogInfo("Created default parking collection for trader: " + traderId);
        }
    }
    
    // Clean up invalid vehicle references (vehicles that were deleted)
    void CleanupInvalidVehicleReferences()
    {
        array<string> traderIds = m_occupiedPositions.GetKeyArray();
        
        for (int i = 0; i < traderIds.Count(); i++)
        {
            string traderId = traderIds[i];
            array<EntityAI> vehicles = m_occupiedPositions.Get(traderId);
            
            for (int j = vehicles.Count() - 1; j >= 0; j--)
            {
                EntityAI vehicle = vehicles[j];
                if (!vehicle)
                {
                    vehicles.Remove(j);
                    GetTraderXLogger().LogDebug("Cleaned up invalid vehicle reference for trader: " + traderId);
                }
            }
        }
    }

    // ===== VEHICLE PARKING RPC METHODS =====

    void GetVehicleParkingDataRequest(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if(type != CallType.Server)
            return;

        Param1<int> data;
        if(!ctx.Read(data))
            return;

        int npcId = data.param1;
        GetTraderXLogger().LogDebug("GetVehicleParkingDataRequest for npcId: " + npcId);

        PlayerBase player = TraderXHelper.GetPlayerByIdentity(sender);
        if(!player)
        {
            GetTraderXLogger().LogError("GetVehicleParkingDataRequest: Player not found");
            return;
        }

        // Get vehicles from parking service
        array<ref TraderXProduct> vehicleProducts = new array<ref TraderXProduct>();
        GetVehiclesFromParkingSpots(npcId, player, vehicleProducts);

        // Send response back to client
        GetRPCManager().SendRPC("TraderX", "GetVehicleParkingDataResponse", new Param1<array<ref TraderXProduct>>(vehicleProducts), true, sender);
    }

    void GetVehiclesFromParkingSpots(int npcId, PlayerBase player, out array<ref TraderXProduct> vehicleProducts)
    {
        GetTraderXLogger().LogDebug("GetVehiclesFromParkingSpots for npcId: " + npcId);

        // Get vehicles in parking spots for this trader
        TraderXVehicleParkingService parkingService = TraderXVehicleParkingService.GetInstance();
        array<EntityAI> occupiedVehicles = parkingService.GetOccupiedVehicles(npcId.ToString());

        // Get trader's categories to match vehicles against sellable products
        TraderXNpc npc = GetTraderXModule().GetSettings().GetNpcById(npcId);
        if (!npc) {
            GetTraderXLogger().LogWarning("NPC not found for id: " + npcId);
            return;
        }

        array<ref TraderXCategory> traderCategories = new array<ref TraderXCategory>();
        foreach(UUID categoryId : npc.categoriesId)
        {
            TraderXCategory category = TraderXCategoryRepository.GetCategoryById(categoryId);
            if (category) {
                traderCategories.Insert(category);
            }
        }

        foreach(EntityAI vehicle: occupiedVehicles)
        {
            if (!vehicle) {
                continue;
            }

            // Check if vehicle is sellable (within range, not occupied, etc.)
            if (!IsVehicleSellableOnServer(vehicle, player)) {
                continue;
            }

            CarScript carScript = CarScript.Cast(vehicle);
            if(carScript && !carScript.IsLastDriver(player))
                continue;

            BoatScript boatScript = BoatScript.Cast(vehicle);
            if(boatScript && !boatScript.IsLastDriver(player))
                continue;

            // Find matching product in trader categories
            TraderXProduct vehicleProduct = FindVehicleProductOnServer(vehicle, traderCategories);
            if (!vehicleProduct) {
                GetTraderXLogger().LogDebug("No sellable product found for vehicle: " + vehicle.GetType());
                continue;
            }

            // Create player item representation with network IDs
            int lowId, highId;
            vehicle.GetNetworkID(lowId, highId);
            int depth = vehicle.GetHierarchyLevel();
            
            TraderXProduct playerVehicleItem = TraderXProduct.CreateAsPlayerItem(vehicle.GetType(), highId, lowId, depth, vehicleProduct, vehicle.GetHealthLevel());
            vehicleProducts.Insert(playerVehicleItem);
            
            GetTraderXLogger().LogDebug("Added sellable vehicle: " + vehicle.GetType() + " (networkId: " + highId + "," + lowId + ")");
        }
    }

    bool IsVehicleSellableOnServer(EntityAI vehicle, PlayerBase player)
    {
        // Check distance to player
        vector playerPos = player.GetPosition();
        vector vehiclePos = vehicle.GetPosition();
        float distance = vector.Distance(playerPos, vehiclePos);
        if (distance > 50.0) {
            GetTraderXLogger().LogDebug("Vehicle too far from player: " + distance + "m");
            return false;
        }

        // Check if vehicle is occupied
        Transport transport = Transport.Cast(vehicle);
        if (transport && transport.IsAnyCrewPresent()) {
            GetTraderXLogger().LogDebug("Vehicle is occupied");
            return false;
        }

        // Check if vehicle is in use (engine running)
        CarScript carScript = CarScript.Cast(vehicle);
        if (carScript && carScript.EngineIsOn()) {
            GetTraderXLogger().LogDebug("Vehicle engine is running");
            return false;
        }

        return true;
    }

    TraderXProduct FindVehicleProductOnServer(EntityAI vehicle, array<ref TraderXCategory> categories)
    {
        string vehicleClassName = vehicle.GetType();
        
        // Search through trader categories for matching vehicle product
        foreach(TraderXCategory category: categories)
        {
            TraderXProduct product = category.FindProductByClassName(vehicleClassName);
            if (product) {
                // Verify this is actually a vehicle product
                if (TraderXVehicleTransactionService.GetInstance().IsVehicleProduct(vehicleClassName)) {
                    return product;
                }
            }
        }
        
        return null;
    }

    void GetVehicleParkingDataResponse(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if(type != CallType.Client)
            return;

        Param1<array<ref TraderXProduct>> data;
        if(!ctx.Read(data))
            return;

        GetTraderXLogger().LogDebug("GetVehicleParkingDataResponse received with " + data.param1.Count() + " vehicles");

        // Forward to SellPageViewController if it exists
        Event_OnVehicleParkingDataReceived.Invoke(data.param1);
    }
}
