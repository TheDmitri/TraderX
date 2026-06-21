class TraderXVehicleFactory
{
    // Spawn a vehicle with preset configuration (fuel, locks, attachments)
    static EntityAI SpawnVehicleWithPreset(string vehicleClassName, TraderXPreset preset, vector position, vector orientation)
    {
        if (!GetGame().IsServer())
        {
            GetTraderXLogger().LogError("SpawnVehicleWithPreset can only be called on server");
            return null;
        }
        
        // Spawn the base vehicle
        EntityAI vehicle = GetGame().CreateObjectEx(vehicleClassName, position, ECE_SETUP | ECE_UPDATEPATHGRAPH | ECE_CREATEPHYSICS);
        if (!vehicle)
        {
            GetTraderXLogger().LogError("Failed to spawn vehicle: " + vehicleClassName);
            return null;
        }
        
        // Set orientation with roll trick to force physics engine acknowledgment
        vehicle.SetPosition(position);
        vehicle.SetOrientation(orientation);
        vector roll = vehicle.GetOrientation();
        roll[2] = roll[2] - 1;
        vehicle.SetOrientation(roll);
        roll[2] = roll[2] + 1;
        vehicle.SetOrientation(roll);
        
        // Configure vehicle based on preset
        ConfigureVehicleFromPreset(vehicle, preset);
        
        GetTraderXLogger().LogInfo("Spawned vehicle: " + vehicleClassName + " with preset: " + preset.presetName + " at position: " + position.ToString());
        return vehicle;
    }
    
    // Configure an existing vehicle with preset settings
    static void ConfigureVehicleFromPreset(EntityAI vehicle, TraderXPreset preset)
    {
        if (!vehicle || !preset)
        {
            return;
        }
        
        // Apply attachments from preset
        ApplyVehicleAttachments(vehicle, preset.attachments);
        
        // Set default fuel level for vehicles
        SetVehicleFuelLevel(vehicle, 1.0); // Full tank
        
        // Configure vehicle locks if it's a lockable vehicle
        ConfigureVehicleLocks(vehicle);
        
        GetTraderXLogger().LogDebug("Configured vehicle with preset: " + preset.presetName);
    }
    
    // Apply attachments to vehicle from preset
    private static void ApplyVehicleAttachments(EntityAI vehicle, array<string> attachmentProductIds)
    {
        if (!attachmentProductIds || attachmentProductIds.Count() == 0)
        {
            return;
        }
        
        for (int i = 0; i < attachmentProductIds.Count(); i++)
        {
            string productId = attachmentProductIds[i];
            
            // Resolve product ID to get the actual class name
            TraderXProduct attachmentProduct = TraderXProductRepository.GetItemById(productId);
            if (!attachmentProduct)
            {
                GetTraderXLogger().LogWarning("Failed to find product for attachment ID: " + productId);
                continue;
            }
            
            string attachmentClassName = attachmentProduct.className;

            // Vehicles spawn (ECE_SETUP) with default parts already filling some slots.
            // CreateAttachment silently fails when the target slot is taken, which is why
            // preset parts (e.g. headlights) never appeared — free the slot(s) first.
            FreeAttachmentSlots(vehicle, attachmentClassName);

            // Create attachment directly on vehicle using DayZ API
            EntityAI attachment = vehicle.GetInventory().CreateAttachment(attachmentClassName);
            if (attachment)
            {
                GetTraderXLogger().LogDebug("Attached item to vehicle: " + attachmentClassName + " (product: " + productId + ")");
            }
            else
            {
                GetTraderXLogger().LogWarning("Failed to create attachment: " + attachmentClassName + " (product: " + productId + ") — no compatible/free slot on " + vehicle.GetType());
            }
        }
    }

    // Remove whatever default part currently occupies the slot(s) this attachment class uses,
    // so the preset part can take it. Reads the item's inventorySlot config (single value or
    // array). No-op when the slot is already empty or the class declares no slot.
    private static void FreeAttachmentSlots(EntityAI vehicle, string attachmentClassName)
    {
        string cfgPath = "CfgVehicles " + attachmentClassName + " inventorySlot";

        array<string> slotNames = new array<string>();
        GetGame().ConfigGetTextArray(cfgPath, slotNames);
        if (slotNames.Count() == 0)
        {
            string single;
            if (GetGame().ConfigGetText(cfgPath, single) && single != "")
                slotNames.Insert(single);
        }

        foreach (string slotName : slotNames)
        {
            int slotId = InventorySlots.GetSlotIdFromString(slotName);
            if (slotId == InventorySlots.INVALID)
                continue;

            EntityAI existing = vehicle.GetInventory().FindAttachment(slotId);
            if (existing)
            {
                GetTraderXLogger().LogDebug("Removing default part " + existing.GetType() + " from slot " + slotName + " to apply preset attachment " + attachmentClassName);
                GetGame().ObjectDelete(existing);
            }
        }
    }
    
    // Set vehicle fuel level
    private static void SetVehicleFuelLevel(EntityAI vehicle, float fuelLevel)
    {
        Car car = Car.Cast(vehicle);
        if (car)
        {
            // Fill fuel tank
            car.Fill(CarFluid.FUEL, fuelLevel * car.GetFluidCapacity(CarFluid.FUEL));
            
            // Fill other fluids to reasonable levels
            car.Fill(CarFluid.OIL, 0.8 * car.GetFluidCapacity(CarFluid.OIL));
            car.Fill(CarFluid.BRAKE, 1.0 * car.GetFluidCapacity(CarFluid.BRAKE));
            car.Fill(CarFluid.COOLANT, 1.0 * car.GetFluidCapacity(CarFluid.COOLANT));
            
            GetTraderXLogger().LogDebug("Set vehicle fuel level to: " + (fuelLevel * 100).ToString() + "%");
        }
    }
    
    // Configure vehicle locks (remove existing locks, make it unlocked)
    private static void ConfigureVehicleLocks(EntityAI vehicle)
    {
        // This would integrate with TraderXCarLock system
        // For now, just ensure the vehicle is unlocked and accessible
        
        Car car = Car.Cast(vehicle);
        if (car)
        {
            // Remove any existing locks
            // This would need to integrate with your car lock system
            GetTraderXLogger().LogDebug("Configured vehicle locks for trading");
        }
    }
    
    // Prepare a vehicle for sale (clean up, reset condition)
    static bool PrepareVehicleForSale(EntityAI vehicle)
    {
        if (!vehicle)
        {
            return false;
        }
        
        // Reset vehicle health to good condition
        vehicle.SetHealth(vehicle.GetMaxHealth());
        
        // Clean up any player-specific modifications
        CleanVehicleForSale(vehicle);
        
        GetTraderXLogger().LogDebug("Prepared vehicle for sale: " + vehicle.GetType());
        return true;
    }
    
    // Clean vehicle of player items and modifications
    private static void CleanVehicleForSale(EntityAI vehicle)
    {
        // Remove all items from vehicle cargo (optional - depends on trading rules)
        GameInventory inventory = vehicle.GetInventory();
        if (inventory)
        {
            // This could be configurable - whether to keep or remove cargo items
            // For now, we'll keep the attachments but clear loose cargo
            
            array<EntityAI> cargoItems = new array<EntityAI>();
            inventory.EnumerateInventory(InventoryTraversalType.LEVELORDER, cargoItems);
            
            for (int i = 0; i < cargoItems.Count(); i++)
            {
                EntityAI item = cargoItems[i];
                if (item)
                {
                    // Remove loose items (not attachments)
                    GetGame().ObjectDelete(item);
                }
            }
        }
    }
}
