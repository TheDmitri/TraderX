class TraderXItemValidator
{
  static bool IsWearByPlayer(EntityAI entity)
  {
    InventoryLocation il = new InventoryLocation;
    entity.GetInventory().GetCurrentInventoryLocation( il );
    if( !il.IsValid() )return true;
      // check the direct parent (clothing will be the parent if not on man attachment)
    if (il.GetParent() && !il.GetParent().IsInherited(Man))
      return false;

    // items in hands are not worn
    if (il.GetType() == InventoryLocationType.HANDS)
      return false;

    return true;
  }
  
  static bool ShouldSkipItem(ItemBase item)
  {
    if (item == null || item.IsLockedInSlot() || IsWearByPlayer(item) || Weapon_Base.Cast(item.GetHierarchyParent()) != null)
      return true;
  
    return false;
  }

  static ItemBase SetHealthLevel(ItemBase item, int level)
  {
    item.SetHealth("","",TraderXQuantityManager.GetDamageStateFromLevel(level)*item.GetMaxHealth());
    return item;
  }
  
  static bool IsMatchingItem(ItemBase item, string className, int health = -1)
  {
      if (item.GetType() != className)
          return false;
  
      if (item.IsKindOf("Edible_Base"))
      {
          Edible_Base edible = Edible_Base.Cast(item);
          if (edible.HasFoodStage() && edible.GetFoodStageType() != FoodStageType.RAW)
              return false;
      }
  
      if (!item.IsInherited(SurvivorBase) && (item.GetHealthLevel() == health || health == -1))
          return true;
  
      return false;
  }
}
