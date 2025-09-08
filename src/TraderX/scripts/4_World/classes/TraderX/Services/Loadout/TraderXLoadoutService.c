class TraderXLoadoutService
{
    static ref TraderXLoadoutService m_instance;

    static TraderXLoadoutService GetInstance()
    {
        if (!m_instance)
        {
            m_instance = new TraderXLoadoutService();
        }
        return m_instance;
    }

    void RemovePlayerClothes(PlayerBase player)
    {
        array<EntityAI> itemsArray = new array<EntityAI>();
        player.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, itemsArray);
        for (int i = itemsArray.Count() - 1; i >= 0; i--)
        {
            if(!itemsArray[i].IsInherited(PlayerBase))
                GetGame().ObjectDelete(itemsArray[i]);
        }
    }

    void EquipPlayer(PlayerBase player, array<ref TraderXLoadoutItem> loadoutItems)
    {
        foreach(TraderXLoadoutItem loadoutItem: loadoutItems)
        {
            EntityAI entityToSpawn;
            if(loadoutItem.slotName == "Hand")
            {
                entityToSpawn = player.GetHumanInventory().CreateInHands(loadoutItem.className);
            }
            else if(loadoutItem.slotName != string.Empty)
            {
                int slotId = InventorySlots.GetSlotIdFromString(loadoutItem.slotName);
                entityToSpawn = player.GetInventory().CreateAttachmentEx(loadoutItem.className, slotId);
            }
            else
            {
                entityToSpawn = player.GetInventory().CreateInInventory(loadoutItem.className);
            }

            if(entityToSpawn)
            {
                if(loadoutItem.quantity != -1)
                    SetQuantity(ItemBase.Cast(entityToSpawn), loadoutItem.quantity);

                entityToSpawn = SpawnAttachments(entityToSpawn, loadoutItem);
            }
        }
    }

    static ItemBase SetQuantity(ItemBase item, int amount)
    {
        Magazine mag = Magazine.Cast(item);
        if (mag){
            mag.ServerSetAmmoCount(amount);
        }
        else{
            item.SetQuantity(amount);
        }   
            
        return item;
    }

    EntityAI SpawnAttachments(EntityAI entity, TraderXLoadoutItem loadoutItem)
    {
        if(loadoutItem.attachments.Count() > 0)
        {
            foreach(TraderXLoadoutAttachment attachment: loadoutItem.attachments)
            {
                Weapon_Base wpn = Weapon_Base.Cast(entity);

                EntityAI attach = EntityAI.Cast(GetGame().CreateObject(attachment.className , "0 0 0"));

                EntityAI entAttachment;

                if(wpn && attach && attach.IsInherited(Magazine) && !attach.IsInherited(Ammunition_Base))
                {
                    entAttachment = wpn.SpawnAttachedMagazine(attachment.className);
                }
                else
                {
                    entAttachment = entity.GetInventory().CreateAttachment(attachment.className);
                }

                GetGame().ObjectDelete(attach);
                
                if(entAttachment)
                {
                    if(attachment.quantity != -1)
                        SetQuantity(ItemBase.Cast(entAttachment), attachment.quantity);
                }
            }
        }

        return entity;
    }
}