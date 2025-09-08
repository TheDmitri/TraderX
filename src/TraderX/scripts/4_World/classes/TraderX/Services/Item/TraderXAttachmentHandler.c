class TraderXAttachmentHandler
{
  static TStringArray GetAttachmentSlots(ItemBase parent)
    {
        TStringArray searching_in = new TStringArray();
		searching_in.Insert(CFG_VEHICLESPATH);
		searching_in.Insert(CFG_WEAPONSPATH);
		searching_in.Insert(CFG_MAGAZINESPATH);

        TStringArray attachmentSlots = new TStringArray();
        foreach (string path : searching_in)
        {
            GetGame().ConfigGetTextArray(path + " " + parent.GetType() + " attachments", attachmentSlots);
            if(attachmentSlots.Count() > 0){
                break;
            }
        }

        if(parent.IsInherited(Weapon_Base)){
            attachmentSlots.Insert("magazine");
        }
        
        return attachmentSlots;
    }

    static bool CanAttachToSlot(ItemBase attachment, string slotName)
    {
        TStringArray searching_in = new TStringArray;
		searching_in.Insert(CFG_VEHICLESPATH);
		searching_in.Insert(CFG_WEAPONSPATH);
		searching_in.Insert(CFG_MAGAZINESPATH);

        foreach (string path : searching_in)
        {
            TStringArray slotNames = new TStringArray();
            GetGame().ConfigGetTextArray(path + " " + attachment.GetType() + " inventorySlot", slotNames);

            foreach(string slot : slotNames)
            {
                if(CF_String.EqualsIgnoreCase(slot,slotName))
                    return true;
            }
        }

        return false;
    }

    static bool TryAttachItem(EntityAI parent, EntityAI newAttachment)
    {
        EntityAI createdEntity;
        bool hasBeenAdded = false;
        array<string> slotNames = GetAttachmentSlots(parent);
        for (int i = 0; i < slotNames.Count(); i++)
        {
            string slotName = slotNames[i];
            if (CanAttachToSlot(newAttachment, slotName))
            {
                EntityAI existingAttachment = parent.FindAttachmentBySlotName(slotName);
                if (existingAttachment)
                {
                    // Remove existing attachment
                    GetGame().ObjectDelete(existingAttachment);
                }

                Weapon_Base wpn = Weapon_Base.Cast(parent);
                if(wpn && newAttachment && newAttachment.IsInherited(Magazine) && !newAttachment.IsInherited(Ammunition_Base))
                {
                    // Attach new mag
                    createdEntity = wpn.SpawnAttachedMagazine(newAttachment.GetType());
                }
                else
                {
                    // Attach new item
                    createdEntity = parent.GetInventory().CreateAttachment(newAttachment.GetType());
                }
                break;
            }
        }

        if(createdEntity)
            hasBeenAdded = true;

        return hasBeenAdded;
    }

  static array<EntityAI> GetAttachments(EntityAI item)
  {
    array<EntityAI> attachments = new array<EntityAI>();
      
    Weapon_Base wpn = Weapon_Base.Cast(item);
      
    if (wpn && !StaticTraderXCoreLists.blackListWeaponsFromAttachments.Find(wpn.GetType()) != -1)
    {
      int slotCountCorrect = wpn.GetSlotsCountCorrect();
      for (int i = 0; i < slotCountCorrect; i++)
      {
        int slotId = wpn.GetInventory().GetAttachmentSlotId(i);
        EntityAI attachment = EntityAI.Cast(wpn.GetInventory().FindAttachment(slotId));
              
        if (attachment){
          attachments.Insert(attachment);
        }
      }
    }
      
    return attachments;
  }

  static void RecreateAllAttachments(PlayerBase player, array<EntityAI> attachments)
  {
    for(int i = 0; i< attachments.Count() ; i++)
  	{
  		ItemBase item = ItemBase.Cast(attachments[i]);
  		if(item)
  			TraderXItemFactory.CreateInInventory(player, item.GetType(), TraderXQuantityManager.GetItemAmount(item), item.GetHealthLevel());
  	}
  }
}
