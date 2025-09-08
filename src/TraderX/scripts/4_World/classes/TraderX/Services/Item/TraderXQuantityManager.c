class TraderXQuantityManager
{
  static int GetItemAmount(ItemBase item)
  {
    int amount = 1;

    if (item.HasQuantity())
    {
      Magazine mag;
      if (Class.CastTo(mag, item))
        amount = mag.GetAmmoCount();
      else
        amount = item.GetQuantity();
    }
    return amount;
  }

  static ItemBase AddQuantity(ItemBase item, int quantity)
  {
    Magazine mag = Magazine.Cast(item);
    if (mag)
        mag.ServerSetAmmoCount(mag.GetAmmoCount() + quantity);
    else
      item.SetQuantity((QuantityConversions.GetItemQuantity(item) + quantity));
    return item;
  }

  static ItemBase SetQuantity(ItemBase item, int amount)
  {
    Magazine mag = Magazine.Cast(item);
    if (mag)
        mag.ServerSetAmmoCount(amount);
    else
      item.SetQuantity(amount);
    return item;
  }

  static int GetItemVolumePerCent(ItemBase item)
  {
    int currentVolume = TraderXQuantityManager.GetItemAmount(item);
    int maxVolume = TraderXQuantityManager.GetMaxVolume(item);
    return Math.Round((currentVolume / maxVolume) * 100);
  }

  static int GetMaxVolume(ItemBase item)
  {
    float currentQuantity, minQuantity, maxQuantity;
    QuantityConversions.GetItemQuantity(item, currentQuantity, minQuantity, maxQuantity);

    if(maxQuantity == 0){
      maxQuantity = 1;
    }
        
    return maxQuantity;
  }

  static int GetMaxItemQuantityClient(EntityAI ent)
  {
    ItemBase item;
    if(Class.CastTo(item, ent))
    {
      float currentQuantity, minQuantity, maxQuantity;
      QuantityConversions.GetItemQuantity(item, currentQuantity, minQuantity, maxQuantity);

      if(maxQuantity == 0){
        maxQuantity = 1;
      }
            
      return maxQuantity;
    }
    return 0;
  }

  static int GetMaxItemQuantityServer(string className)
  {
    EntityAI ent = EntityAI.Cast(GetGame().CreateObject(className , "0 0 0"));
    if(!ent)
      return 0;

    ItemBase item;
    if(Class.CastTo(item, ent))
    {
      float currentQuantity, minQuantity, maxQuantity;
      QuantityConversions.GetItemQuantity(item, currentQuantity, minQuantity, maxQuantity);

      if(maxQuantity == 0){
        maxQuantity = 1;
      }
            
      GetGame().ObjectDelete(item);
      return maxQuantity;
    }

    GetGame().ObjectDelete(ent);
    return 0;
  }

  static bool ContainsIgnoreCase(string source, string key)
  {
    source.ToLower();
    key.ToLower();

    return source.Contains(key);
  }

  static float GetDamageStateFromLevel(int level)
  {
  	switch(level)
  	{
  		case GameConstants.STATE_PRISTINE:
  			return GameConstants.DAMAGE_PRISTINE_VALUE;
  			break;

  		case GameConstants.STATE_BADLY_DAMAGED:
  			return GameConstants.DAMAGE_BADLY_DAMAGED_VALUE;
  			break;

  		case GameConstants.STATE_DAMAGED:
  			return GameConstants.DAMAGE_DAMAGED_VALUE;
  			break;

  		case GameConstants.STATE_WORN:
  			return GameConstants.DAMAGE_WORN_VALUE;
  			break;

  		case GameConstants.STATE_RUINED:
  			return GameConstants.DAMAGE_RUINED_VALUE;
  			break;

  		default: return GameConstants.DAMAGE_PRISTINE_VALUE;
  			break;
  	}
	   return GameConstants.DAMAGE_PRISTINE_VALUE;
  }

  static string GetFormattedMoneyAmount(int moneyAmount)
  {
    string moneyReversed = "";
    string strNumber = moneyAmount.ToString();

    int processedCount = 0;
    for (int i = (strNumber.Length() - 1); i >= 0; i--)
    {
      moneyReversed += strNumber[i];
      processedCount += 1;
      if ((processedCount % 3) == 0 && i != 0)
      {
        moneyReversed += ",";
      }
    }

    string money = "";
    for (int j = (moneyReversed.Length() - 1); j >= 0; j--)
    {
      money += moneyReversed[j];
    }

    return money;
  }

  static void EmptyMag(PlayerBase player, Magazine mag)
    {
        #ifdef TRADERXDEBUG
        GetTraderXLogger().LogInfo("EmptyMag function:");
        #endif
        float dmg;
        string ammoType;
        int quantity = mag.GetAmmoCount();

        // Gather ammo types, quantites and health
        TStringArray ammoKeys = new TStringArray;
        TStringArray ammoTypes = new TStringArray;
        TFloatArray damages = new TFloatArray;
        TIntArray quantites = new TIntArray;
        for (int i = 0; i < quantity; ++i)
        {
            if ( !mag.ServerAcquireCartridge(dmg,ammoType) )
                break;

            float roundDmg = Math.Round(dmg * 1000)/1000;
            string ammoKey = string.Format("%1_%2", ammoType, roundDmg);
            int bulletIndex = ammoKeys.Find(ammoKey);
            if (bulletIndex == -1)
            {
                ammoKeys.Insert(ammoKey);
                damages.Insert(roundDmg);
                quantites.Insert(1);
                ammoTypes.Insert(ammoType);
                continue;
            }
            quantites.Set(bulletIndex, quantites.Get(bulletIndex) + 1);
        }

        // Move to inventory, or ground if inventory full
        array< Magazine > piles;
        Magazine magazinePile;
        for (int j = 0; j < ammoKeys.Count(); ++j)
        {
            float bulletsDamage = damages.Get(j);
            int bulletsCount = quantites.Get(j);
            string bulletType = ammoTypes.Get(j);

            CreateMagazinePilesFromBullet(player, bulletType,bulletsCount,bulletsDamage);
        }
    }

  static array<Magazine> CreateMagazinePilesFromBullet(PlayerBase player,string bullet_type, float quantity, float bulletsDamage )
  {
      #ifdef TRADERXDEBUG
      GetTraderXLogger().LogInfo("CreateMagazinePilesFromBullet:"+quantity);
      #endif
      array<Magazine> items = new array<Magazine>;
      string item_name;
      if( !g_Game.ConfigGetText("cfgAmmo " + bullet_type + " spawnPileType", item_name) )
          return items;

      float stack_size = g_Game.ConfigGetInt("cfgMagazines " + item_name + " count");

      if(stack_size > 0)
      {
          Magazine pile;
          int piles_count = Math.Floor(quantity/stack_size);
          int rest = quantity - (piles_count*stack_size);

          for ( int i = 0; i < piles_count; i++ )
          {
              pile = Magazine.Cast(TraderXItemFactory.CreateInInventory(player,item_name,stack_size, true));
              items.Insert(pile);
              SetHealthFromDamage(pile, bulletsDamage);
          }
          if ( rest > 0)
          {
              pile = Magazine.Cast(TraderXItemFactory.CreateInInventory(player,item_name, rest, true));
              items.Insert(pile);
              SetHealthFromDamage(pile, bulletsDamage);
          }
      }
      return items;
  }

  static void SetHealthFromDamage(Magazine pile, float bulletsDamage)
  {
      float bulletsHealth = pile.GetMaxHealth("", "") * (1.0 - bulletsDamage);
      pile.SetHealth( "", "", bulletsHealth );
  }

  static bool CanLoadMagazine(ItemBase target, ItemBase item)
	{
		Magazine ammo;
		Magazine mag;
		return ( Class.CastTo(ammo, target) && Class.CastTo(mag, item) && mag.GetAmmoCount() < mag.GetAmmoMax() && mag.IsCompatiableAmmo( ammo ) );
	}

  static bool IsMagazine(string className)
  {
    bool isMag = false;
    ItemBase item = ItemBase.Cast(GetGame().CreateObject(className, vector.Zero));
    Magazine mag;
    Ammunition_Base ammo;

    if(CastTo(mag, item) && !CastTo(ammo, item))
      isMag = true;

    GetGame().ObjectDelete(item);

    return isMag;
  }

  static int GetTotalQuantityOfItem(PlayerBase player, string className, int health = TraderXProductState.ALL_STATE)
  {
      if(!player)
        return 0;

  		int quantity = 0;
  		array<EntityAI> itemsArray = TraderXInventoryManager.GetItemsArray(player);
      foreach(EntityAI entity: itemsArray)
      {
        if (CF_String.EqualsIgnoreCase(entity.GetType(),className))
        {
          ItemBase item = ItemBase.Cast(entity);
          if(TraderXItemValidator.ShouldSkipItem(item))
            continue;

          if(item.GetHealthLevel() == health || health == TraderXProductState.ALL_STATE)
            quantity+=GetItemAmount(item);
        }
      }

  		return quantity;
  }

  static ItemBase CheckForStackableItem(PlayerBase player, string className, int quantity = 1)
  {
    GetTraderXLogger().LogDebug(string.Format("[STACKING] CheckForStackableItem called for %1 x%2", className, quantity.ToString()));
    
    ItemBase item;
    array<EntityAI> itemsArray = TraderXInventoryManager.GetItemsArray(player);
    array<ItemBase> stackableItems = new array<ItemBase>;
    
    GetTraderXLogger().LogDebug(string.Format("[STACKING] Found %1 items in player inventory", itemsArray.Count().ToString()));
    
    for (int i = 0; i < itemsArray.Count(); ++i)
    {
      item = ItemBase.Cast(itemsArray[i]);
      if (item && CF_String.EqualsIgnoreCase(item.GetType(), className))
      {
        string itemType = item.GetType();
        GetTraderXLogger().LogDebug(string.Format("[STACKING] Checking item %1/%2: %3 (looking for %4)", (i+1).ToString(), itemsArray.Count().ToString(), itemType, className));
        
        GetTraderXLogger().LogDebug(string.Format("[STACKING] Found matching item: %1", className));
        
        // Handle magazines/ammunition differently
        Magazine mag = Magazine.Cast(item);
        if (mag)
        {
          int currentAmmo = mag.GetAmmoCount();
          int maxAmmo = mag.GetAmmoMax();
          
          GetTraderXLogger().LogDebug(string.Format("[STACKING] Magazine found: %1/%2 ammo, trying to add %3", 
              currentAmmo.ToString(), maxAmmo.ToString(), quantity.ToString()));
          
          if ((currentAmmo + quantity) <= maxAmmo)
          {
            GetTraderXLogger().LogDebug(string.Format("[STACKING] Magazine can accept %1 more rounds", quantity.ToString()));
            stackableItems.Insert(item);
          }
          else
          {
            GetTraderXLogger().LogDebug(string.Format("[STACKING] Magazine cannot accept %1 more rounds (would exceed max)", quantity.ToString()));
          }
        }
        else
        {
          // Handle regular stackable items
          float currentQuantity, minQuantity, maxQuantity;
          QuantityConversions.GetItemQuantity(item, currentQuantity, minQuantity, maxQuantity);

          GetTraderXLogger().LogDebug(string.Format("[STACKING] Regular item found: %1/%2 quantity, trying to add %3", 
              currentQuantity.ToString(), maxQuantity.ToString(), quantity.ToString()));

          if (maxQuantity == 0){
            currentQuantity = 1;
            maxQuantity = 1;
          }

          if ((currentQuantity + quantity) <= maxQuantity)
          {
            GetTraderXLogger().LogDebug(string.Format("[STACKING] Regular item can accept %1 more units", quantity.ToString()));
            stackableItems.Insert(item);
          }
          else
          {
            GetTraderXLogger().LogDebug(string.Format("[STACKING] Regular item cannot accept %1 more units (would exceed max)", quantity.ToString()));
          }
        }
      }
    }

    if (stackableItems.Count() > 0){
      item = stackableItems[0];
      GetTraderXLogger().LogDebug(string.Format("[STACKING] Adding %1 to existing stack", quantity.ToString()));
      return AddQuantity(item, quantity);
    }

    GetTraderXLogger().LogDebug("[STACKING] No stackable items found, returning null");
    return null;
  }
}
