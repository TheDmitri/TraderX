class TraderXInventoryManager
{
  static void PlayMenuSound(string soundSet)
  {
    if(!GetGame().IsClient())
      return;

    EffectSound sound;
    PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
    player.PlaySoundSet(sound, soundSet, 0.0, 0.0);
  }

  static void GetPlayerEntitiesFromSlotId(PlayerBase player, int slotId, out array<EntityAI> playerItems)
    {
        if(InventorySlots.HANDS == slotId){
            EntityAI entHand = player.GetHumanInventory().GetEntityInHands();
            if(entHand)
                playerItems.Insert(entHand);
            return;
        }

        EntityAI mainEntity = player.GetInventory().FindAttachment(slotId);
        if(!mainEntity){
          return;
        }
        array<EntityAI> tplayerItems = new array<EntityAI>();
		    mainEntity.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER,tplayerItems);
        for(int i = 0; i < tplayerItems.Count(); i++)
        {
            EntityAI cargoEnt = tplayerItems[i];
            if(!cargoEnt)
              continue;

            if((slotId == InventorySlots.SHOULDER || slotId == InventorySlots.MELEE) && mainEntity == cargoEnt){
              playerItems.Insert(cargoEnt);
              continue;
            }
            
            if(!TraderXItemValidator.ShouldSkipItem(ItemBase.Cast(cargoEnt))){
              playerItems.Insert(cargoEnt);
            }
        }
    }

    static void GetEntitiesChildren(EntityAI entity, out array<EntityAI> children)
    {
        array<EntityAI> tEntityItems = new array<EntityAI>();
        if(!entity){
            return;
        }

		entity.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER,tEntityItems);
        for(int i = 0; i < tEntityItems.Count(); i++)
        {
            EntityAI cargoEnt = tEntityItems[i];
            if(cargoEnt && cargoEnt != entity){
                children.Insert(cargoEnt);
            }
        }
    }

    static string GetEntityHierarchy(EntityAI ent)
    {
        string hierarchy = "";
        EntityAI parent = ent.GetHierarchyParent();
        while(parent)
        {
            hierarchy = parent.GetDisplayName() + " > " + hierarchy;
            parent = parent.GetHierarchyParent();
        }
        return hierarchy;
    }

  static array<EntityAI> GetItemsArray(PlayerBase player)
  {
    array<EntityAI> itemsArray = new array<EntityAI>();
    player.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, itemsArray);
    return itemsArray;
  }

  static bool RemoveOurProduct(PlayerBase player, string className, int quantity, int health = -1)
  {
    if (quantity < 0)
      quantity = TraderXQuantityManager.GetMaxItemQuantityServer(className);
  
    int itemSpecificCount = TraderXQuantityManager.GetTotalQuantityOfItem(player, className, health);
  
    if (itemSpecificCount < quantity || itemSpecificCount == 0)
    {
      return false;
    }
  
    array<EntityAI> itemsArray = GetItemsArray(player);
    foreach (EntityAI entity : itemsArray)
    {
      ItemBase item = ItemBase.Cast(entity);
          
      if (TraderXItemValidator.ShouldSkipItem(item))
        continue;
  
      if (TraderXItemValidator.IsMatchingItem(item, className, health))
      {
        quantity = RemoveItem(player, item, quantity);

        if(quantity <= 0)
          return true;
      }
    }
  
    return false;
  }

  static int RemoveItem(PlayerBase player, ItemBase item, int quantity, bool removeAttachment = false)
  {
      if(!removeAttachment)
      {
        array<EntityAI> attachments = TraderXAttachmentHandler.GetAttachments(item);
  
        if (attachments.Count() > 0)
          TraderXAttachmentHandler.RecreateAllAttachments(player, attachments);
      }
  
    if (QuantityConversions.HasItemQuantity(item) == 0 || (item.IsInherited(Magazine) && !item.IsInherited(Ammunition_Base)))
    {
      Magazine mag = Magazine.Cast(item);
        if(mag)
          TraderXQuantityManager.EmptyMag(player, mag);

      GetTraderXLogger().LogDebug(string.Format("Remove item 1"));
  
        GetGame().ObjectDelete(item);
        quantity--;
    }
    else
    {
      int itemAmount = TraderXQuantityManager.GetItemAmount(item);
  
      if (itemAmount > quantity && quantity != 0)
      {
        item = TraderXQuantityManager.AddQuantity(item, quantity * -1);

        quantity = 0;
      }
      else
      {
        quantity -= itemAmount;
        GetGame().ObjectDelete(item);
      }
    }

    return quantity;
  }

  static void CheckEntityNetworkId(PlayerBase player)
  {
    array<EntityAI> itemsArray = GetItemsArray(player);
    foreach (EntityAI entity : itemsArray)
    {
      string className = entity.GetType();
      int highId = -1;
      int lowId = -1;
      entity.GetNetworkID(lowId, highId);
    }
  }

  static void RetrieveNetworkIdFromClassNameInPlayer(PlayerBase player, string className, out int lowId, out int highId)
  {
    array<EntityAI> itemsArray = GetItemsArray(player);
    foreach (EntityAI entity : itemsArray)
    {
      string entClassName = entity.GetType();
      entClassName.ToLower();

      if(entClassName == className){
        entity.GetNetworkID(lowId, highId);
        return;
      }
    }
  }

  static string GetSlotForPlayerPreview(string className)
  {
    TStringArray classnameArr = new TStringArray;
    string Vpath = CFG_VEHICLESPATH + " " + className + " inventorySlot";
    if ( GetGame().ConfigIsExisting( Vpath ) )
        g_Game.ConfigGetTextArray( Vpath , classnameArr);

    foreach(string classname: classnameArr)
    {
      foreach(int idx, string playerAtt: StaticTraderXCoreLists.playerAttachments)
      {
        if(classname == playerAtt)
            return playerAtt;
      }
    }
    return string.Empty;
  }
}
