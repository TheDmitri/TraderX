class TraderXItemFactory
{
  static ItemBase CreateInCargos(PlayerBase player, string className, int quantity = 1)
  {
    ItemBase item;
    array<EntityAI> itemsArray = TraderXInventoryManager.GetItemsArray(player);
    for(int i = 0; i < itemsArray.Count(); i++)
    {
      if(!Class.CastTo(item, itemsArray[i]))
        continue;

      EntityAI newItem = EntityAI.Cast(item.GetInventory().CreateInInventory(className));
      if (newItem)
        return ItemBase.Cast(newItem);
    }

    return null;
  }

  // static ItemBase CreateInInventory(PlayerBase player, string className, int quantity = 1, int healthLevel = TraderXProductState.PRISTINE)
  // {
  // 		if(!player)
  //       return NULL;

  //     GetTraderXLogger().LogDebug("CreateInInventory: " + className);

  //     int maxQuantity = TraderXQuantityManager.GetMaxItemQuantityServer(className);
  //     if(quantity < 0) quantity = maxQuantity;

  //     if(quantity > maxQuantity)
  //     {
  //       int loopCount = quantity / maxQuantity;
  //       quantity -= loopCount * maxQuantity;
  //       GetTraderXLogger().LogDebug("loopCount: " + loopCount.ToString());
  //       GetTraderXLogger().LogDebug("quantity: " + quantity.ToString());
  //       for(int i = 0; i < loopCount; i++)
  //       {
  //         CreateInInventory(player, className, maxQuantity);
  //       }
  //     }

  //     if(quantity != 0){
  //       ItemBase item = TraderXQuantityManager.CheckForStackableItem(player, className, quantity);
  //       if(item)
  //         return item;
  //     }

  // 		ItemBase newItem = ItemBase.Cast(player.GetInventory().CreateInInventory(className));

  //     if(!newItem){
  //       //GetTraderXLogger().LogDebug("Item wasn't able to be spawned in inventory at first attempt");
  //       newItem = CreateInCargos(player, className, quantity);
  //     }

  // 		if (!newItem){
  //       //GetTraderXLogger().LogDebug("item doesn't exist in inv => attempt creation in hand");
  // 			newItem = ItemBase.Cast(player.GetHumanInventory().CreateInHands(className));
  //     }

  // 		if (!newItem){
  //       //GetTraderXLogger().LogDebug("item doesn't exist in inv => attempt creation in ground");
  //       newItem = ItemBase.Cast(player.SpawnEntityOnGroundPos(className, player.GetPosition()));
  //     }

  // 		if (newItem){
  //       GetTraderXLogger().LogDebug("item exist in inventory or ground => ");
  //       newItem = TraderXItemValidator.SetHealthLevel(newItem,healthLevel);
  //       return TraderXQuantityManager.SetQuantity(newItem, quantity);
  //     }

  //     GetTraderXLogger().LogDebug("item doesn't exist at all...");

  // 		return null;
  // }

  static ItemBase CreateInInventory(PlayerBase player, string className, int quantity = 1, int healthLevel = TraderXProductState.PRISTINE)
  {
      if(!player)
          return NULL;

      GetTraderXLogger().LogDebug("CreateInInventory: " + className);

      // Get max stack size for this item
      int maxQuantity = TraderXQuantityManager.GetMaxItemQuantityServer(className);
      if(quantity < 0) quantity = maxQuantity;

      // If quantity is more than max stack size, create multiple stacks
      if(quantity > maxQuantity)
      {
          int fullStacks = quantity / maxQuantity;
          int remainder = quantity % maxQuantity;
          
          // Performance safeguard: limit maximum number of stacks
          const int MAX_STACKS_ALLOWED = 1000; // Reasonable limit for performance
          const int EFFICIENCY_WARNING_THRESHOLD = 100; // Warn if creating more than 100 stacks
          
          if(fullStacks > MAX_STACKS_ALLOWED)
          {
              GetTraderXLogger().LogError(string.Format("[PERFORMANCE] Attempted to create %1 stacks of %2. Maximum allowed: %3. Consider using higher denomination currency items (at least 100x larger, but not more than 1000x larger than current denomination).", fullStacks.ToString(), className, MAX_STACKS_ALLOWED.ToString()));
              return NULL;
          }
          
          if(fullStacks > EFFICIENCY_WARNING_THRESHOLD)
          {
              GetTraderXLogger().LogWarning(string.Format("[PERFORMANCE] Creating %1 stacks of %2. For better performance, consider using currency denominations that are 100-1000 times larger.", fullStacks.ToString(), className));
          }
          
          ItemBase lastCreated = NULL;
          
          // Create full stacks with progress logging for large operations
          for(int i = 0; i < fullStacks; i++)
          {
              lastCreated = CreateSingleItem(player, className, maxQuantity, healthLevel);
              if(!lastCreated)
              {
                  GetTraderXLogger().LogError("Failed to create stack " + (i+1).ToString() + " of " + fullStacks.ToString() + " for " + className);
                  return NULL;
              }
              
              // Log progress for large operations
              if(fullStacks > EFFICIENCY_WARNING_THRESHOLD && (i + 1) % 50 == 0)
              {
                  GetTraderXLogger().LogInfo(string.Format("[PROGRESS] Created %1/%2 stacks of %3", (i + 1).ToString(), fullStacks.ToString(), className));
              }
          }
          
          // Create remaining items if any
          if(remainder > 0)
          {
              lastCreated = CreateSingleItem(player, className, remainder, healthLevel);
          }
          
          return lastCreated;
      }
      
      // Single stack case
      return CreateSingleItem(player, className, quantity, healthLevel);
  }

  // Helper method to create a single item/stack using vanilla DayZ patterns
  private static ItemBase CreateSingleItem(PlayerBase player, string className, int quantity, int healthLevel)
  {
      GetTraderXLogger().LogDebug(string.Format("[STACKING] CreateSingleItem called for %1 x%2", className, quantity.ToString()));
      
      // First try to stack with existing items
      ItemBase existingStack = TraderXQuantityManager.CheckForStackableItem(player, className, quantity);
      if(existingStack)
      {
          GetTraderXLogger().LogDebug(string.Format("[STACKING] Successfully added to existing stack: %1 x%2 (new total: %3)", 
              className, quantity.ToString(), TraderXQuantityManager.GetItemAmount(existingStack).ToString()));
          return existingStack;
      }
      else
      {
          GetTraderXLogger().LogDebug(string.Format("[STACKING] No existing stackable item found for %1 x%2, creating new item", className, quantity.ToString()));
      }
      
      // Try multiple placement strategies following vanilla patterns
      ItemBase newItem = TryCreateInInventoryHierarchy(player, className, quantity, healthLevel);
      if(newItem)
      {
          GetTraderXLogger().LogDebug(string.Format("[STACKING] Successfully created new item: %1 x%2 (amount: %3)", 
              className, quantity.ToString(), TraderXQuantityManager.GetItemAmount(newItem).ToString()));
          return newItem;
      }
      
      GetTraderXLogger().LogError("Failed to create item in any location: " + className);
      return NULL;
  }
  
  // Enhanced creation method following vanilla inventory hierarchy
  private static ItemBase TryCreateInInventoryHierarchy(PlayerBase player, string className, int quantity, int healthLevel)
  {
      ItemBase newItem;
      
      // 1. Try standard inventory (cargo + attachments) - vanilla CreateInInventory pattern
      newItem = ItemBase.Cast(player.GetInventory().CreateInInventory(className));
      if(newItem)
      {
          GetTraderXLogger().LogDebug("Created in standard inventory: " + className);
          return FinalizeItem(newItem, quantity, healthLevel);
      }
      
      // 2. Try specific cargo locations - our existing method
      newItem = CreateInCargos(player, className, quantity);
      if(newItem)
      {
          GetTraderXLogger().LogDebug("Created in cargo: " + className);
          return FinalizeItem(newItem, quantity, healthLevel);
      }
      
      // 3. Try hands - vanilla HumanInventory fallback pattern
      newItem = ItemBase.Cast(player.GetHumanInventory().CreateInHands(className));
      if(newItem)
      {
          GetTraderXLogger().LogDebug("Created in hands: " + className);
          return FinalizeItem(newItem, quantity, healthLevel);
      }
      
      // 4. Try ground placement - vanilla ground placement pattern
      newItem = TryCreateOnGround(player, className, quantity, healthLevel);
      if(newItem)
      {
          GetTraderXLogger().LogDebug("Created on ground: " + className);
          return newItem; // Already finalized in TryCreateOnGround
      }
      
      return NULL;
  }
  
  // Ground placement following vanilla PrepareDropEntityPos pattern
  private static ItemBase TryCreateOnGround(PlayerBase player, string className, int quantity, int healthLevel)
  {
      // Create item at player position first
      vector playerPos = player.GetPosition();
      ItemBase newItem = ItemBase.Cast(player.SpawnEntityOnGroundPos(className, playerPos));
      
      if(newItem)
      {
          // Use vanilla-style ground positioning
          vector transform[4];
          if(GameInventory.PrepareDropEntityPos(player, newItem, transform, false, -1))
          {
              newItem.SetTransform(transform);
              GetTraderXLogger().LogDebug("Positioned item on ground with conflict avoidance: " + className);
          }
          else
          {
              GetTraderXLogger().LogWarning("Ground placement without conflict avoidance: " + className);
          }
          
          return FinalizeItem(newItem, quantity, healthLevel);
      }
      
      return NULL;
  }
  
  // Finalize item with quantity and health
  private static ItemBase FinalizeItem(ItemBase item, int quantity, int healthLevel)
  {
      if(!item)
          return NULL;
      
      TraderXQuantityManager.SetQuantity(item, quantity);
      return TraderXItemValidator.SetHealthLevel(item, healthLevel);
  }

  static ItemBase CreateAsAttachment(ItemBase parent, string className, int quantity, int healthLevel = TraderXProductState.PRISTINE)
  {
    Weapon_Base wpn = Weapon_Base.Cast(parent);
    EntityAI attach = EntityAI.Cast(GetGame().CreateObject(className, "0 0 0"));

    EntityAI entAttachment;

    if(wpn && attach && attach.IsInherited(Magazine) && !attach.IsInherited(Ammunition_Base))
    {
      entAttachment = wpn.SpawnAttachedMagazine(className);
    }
    else
    {
      entAttachment = parent.GetInventory().CreateAttachment(className);
    }

    if(!entAttachment){
      entAttachment = parent.GetInventory().CreateInInventory(className);
    }

    GetGame().ObjectDelete(attach);
                
    if(entAttachment && quantity != -1){
      TraderXQuantityManager.SetQuantity(ItemBase.Cast(entAttachment), quantity);
    }

    return entAttachment;
  }
}
