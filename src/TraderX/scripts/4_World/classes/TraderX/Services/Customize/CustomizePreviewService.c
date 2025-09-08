class CustomizePreviewService
{
    private static ref CustomizePreviewService m_Instance;
    
    // Events for UI coordination
    static ref ScriptInvoker Event_OnPreviewUpdated = new ScriptInvoker();
    static ref ScriptInvoker Event_OnAttachmentsChanged = new ScriptInvoker();
    
    // Current preview state
    private EntityAI m_CurrentPreview;
    private ref array<ref TraderXProduct> m_CurrentAttachments;
    private TraderXProduct m_BaseItem;
    
    void CustomizePreviewService()
    {
        m_CurrentAttachments = new array<ref TraderXProduct>();
    }
    
    void ~CustomizePreviewService()
    {
        CleanupCurrentPreview();
    }
    
    static CustomizePreviewService GetInstance()
    {
        if (!m_Instance)
            m_Instance = new CustomizePreviewService();
        return m_Instance;
    }
    
    // === PREVIEW MANAGEMENT ===
    
    EntityAI CreatePreview(TraderXProduct item)
    {
        // Clean up previous preview
        CleanupCurrentPreview();
        
        // Create new preview entity
        m_CurrentPreview = EntityAI.Cast(GetGame().CreateObjectEx(item.className, vector.Zero, ECE_LOCAL|ECE_NOLIFETIME));
        m_BaseItem = item;
        
        if (m_CurrentPreview)
        {
            Event_OnPreviewUpdated.Invoke(m_CurrentPreview);
        }
        else
        {
            GetTraderXLogger().LogError("CustomizePreviewService::CreatePreview - Failed to create preview for " + item.className);
        }
        
        return m_CurrentPreview;
    }
    
    void CleanupCurrentPreview()
    {
        if (m_CurrentPreview)
        {
            GetGame().ObjectDelete(m_CurrentPreview);
            m_CurrentPreview = null;
        }
        
        m_CurrentAttachments.Clear();
    }
    
    EntityAI GetCurrentPreview()
    {
        return m_CurrentPreview;
    }
    
    TraderXProduct GetBaseItem()
    {
        return m_BaseItem;
    }
    
    // === ATTACHMENT MANAGEMENT ===
    
    bool AddAttachment(TraderXProduct attachment)
    {
        if (!m_CurrentPreview || !attachment)
            return false;
            
        // Check if attachment already exists
        if (HasAttachment(attachment))
        {
            return false;
        }
        
        // Create attachment entity
        EntityAI attachmentEntity = EntityAI.Cast(GetGame().CreateObjectEx(attachment.className, vector.Zero, ECE_LOCAL|ECE_NOLIFETIME));
        if (!attachmentEntity)
        {
            GetTraderXLogger().LogError("CustomizePreviewService::AddAttachment - Failed to create attachment entity: " + attachment.className);
            return false;
        }
        
        // Try to attach to preview
        bool success = TraderXAttachmentHandler.TryAttachItem(m_CurrentPreview, attachmentEntity);
        if (success)
        {
            m_CurrentAttachments.Insert(attachment);
            
            Event_OnAttachmentsChanged.Invoke(m_CurrentAttachments);
            Event_OnPreviewUpdated.Invoke(m_CurrentPreview);
            return true;
        }
        else
        {
            GetGame().ObjectDelete(attachmentEntity);
            return false;
        }
    }
    
    bool RemoveAttachment(TraderXProduct attachment)
    {
        if (!m_CurrentPreview || !attachment)
            return false;
            
        // Find and remove from preview entity
        array<EntityAI> entityAttachments = new array<EntityAI>();
        TraderXInventoryManager.GetEntitiesChildren(m_CurrentPreview, entityAttachments);
        
        bool found = false;
        for (int i = entityAttachments.Count() - 1; i >= 0; i--)
        {
            if (CF_String.EqualsIgnoreCase(entityAttachments[i].GetType(), attachment.className))
            {
                GetGame().ObjectDelete(entityAttachments[i]);
                found = true;
                break;
            }
        }
        
        // Remove from tracking array
        for (int j = m_CurrentAttachments.Count() - 1; j >= 0; j--)
        {
            if (m_CurrentAttachments[j].productId == attachment.productId)
            {
                m_CurrentAttachments.Remove(j);
                break;
            }
        }
        
        if (found)
        {
            Event_OnAttachmentsChanged.Invoke(m_CurrentAttachments);
            Event_OnPreviewUpdated.Invoke(m_CurrentPreview);
        }
        
        return found;
    }
    
    void ClearAllAttachments()
    {
        if (!m_CurrentPreview)
            return;
            
        array<EntityAI> entityAttachments = new array<EntityAI>();
        TraderXInventoryManager.GetEntitiesChildren(m_CurrentPreview, entityAttachments);
        
        foreach (EntityAI attachment : entityAttachments)
        {
            GetGame().ObjectDelete(attachment);
        }
        
        m_CurrentAttachments.Clear();
        
        Event_OnAttachmentsChanged.Invoke(m_CurrentAttachments);
        Event_OnPreviewUpdated.Invoke(m_CurrentPreview);
    }
    
    void ApplyAttachments(array<ref TraderXProduct> attachments)
    {
        if (!m_CurrentPreview)
            return;
            
        // Clear existing attachments first
        ClearAllAttachments();
        
        // Apply new attachments
        foreach (TraderXProduct attachment : attachments)
        {
            AddAttachment(attachment);
        }
    }
    
    bool HasAttachment(TraderXProduct attachment)
    {
        foreach (TraderXProduct currentAttachment : m_CurrentAttachments)
        {
            if (currentAttachment.productId == attachment.productId)
                return true;
        }
        return false;
    }
    
    array<ref TraderXProduct> GetCurrentAttachments()
    {
        return m_CurrentAttachments;
    }
    
    // === VARIANT MANAGEMENT ===
    
    bool SetVariant(TraderXProduct variant)
    {
        if (!variant)
            return false;
            
        // Store current attachments
        array<ref TraderXProduct> currentAttachments = new array<ref TraderXProduct>();
        foreach (TraderXProduct attachment : m_CurrentAttachments)
        {
            currentAttachments.Insert(attachment);
        }
        
        // Create new preview with variant
        EntityAI newPreview = CreatePreview(variant);
        if (!newPreview)
            return false;
            
        // Re-apply attachments to new variant
        ApplyAttachments(currentAttachments);
        
        // Notify that preview has been updated
        Event_OnPreviewUpdated.Invoke(m_CurrentPreview);
        
        return true;
    }
    
    // === PRESET MANAGEMENT ===
    
    bool ApplyPreset(TraderXPreset preset)
    {
        if (!preset)
            return false;
            
        // If preset has different base item, switch variant first
        if (preset.productId != m_BaseItem.productId)
        {
            TraderXProduct variantItem = TraderXProductRepository.GetItemById(preset.productId);
            if (!variantItem)
            {
                GetTraderXLogger().LogError("CustomizePreviewService::ApplyPreset - Variant item not found: " + preset.productId);
                return false;
            }
            
            if (!SetVariant(variantItem))
                return false;
        }
        else
        {
            // Clear attachments if using same base item
            ClearAllAttachments();
        }
        
        // Apply preset attachments
        array<ref TraderXProduct> presetAttachments = new array<ref TraderXProduct>();
        map<string, int> attachmentCounts = new map<string, int>();
        
        // Count occurrences of each productId
        foreach (string productId : preset.attachments)
        {
            if (attachmentCounts.Contains(productId))
            {
                attachmentCounts.Set(productId, attachmentCounts.Get(productId) + 1);
            }
            else
            {
                attachmentCounts.Set(productId, 1);
            }
        }
        
        // Create attachment products with proper multipliers
        foreach (string prodId, int count : attachmentCounts)
        {
            TraderXProduct attachment = TraderXProductRepository.GetItemById(prodId);
            if (attachment)
            {
                attachment.SetMultiplier(count);
                presetAttachments.Insert(attachment);
            }
        }
        
        ApplyAttachments(presetAttachments);
        return true;
    }
    
    // === UTILITY METHODS ===
    
    string GetItemDescription()
    {
        if (!m_CurrentPreview)
            return string.Empty;

        InventoryItem iItem = InventoryItem.Cast(m_CurrentPreview);
        if (iItem)
            return TraderXCoreUtils.TrimUnt(iItem.GetTooltip());

        return string.Empty;
    }
    
    void SynchronizeAttachments()
    {
        if (!m_CurrentPreview)
            return;
            
        // Get actual entity attachments
        array<EntityAI> entityAttachments = new array<EntityAI>();
        TraderXInventoryManager.GetEntitiesChildren(m_CurrentPreview, entityAttachments);
        
        // Create temporary array for items to remove
        array<ref TraderXProduct> itemsToRemove = new array<ref TraderXProduct>();
        
        // Check if each tracked attachment still exists
        foreach (TraderXProduct trackedItem : m_CurrentAttachments)
        {
            bool found = false;
            foreach (EntityAI entityItem : entityAttachments)
            {
                if (CF_String.EqualsIgnoreCase(entityItem.GetType(), trackedItem.className))
                {
                    found = true;
                    break;
                }
            }
            
            if (!found)
                itemsToRemove.Insert(trackedItem);
        }
        
        // Remove items that no longer exist
        foreach (TraderXProduct itemToRemove : itemsToRemove)
        {
            int idx = m_CurrentAttachments.Find(itemToRemove);
            if (idx != -1)
                m_CurrentAttachments.Remove(idx);
        }
        
        if (itemsToRemove.Count() > 0)
        {
            Event_OnAttachmentsChanged.Invoke(m_CurrentAttachments);
        }
    }
}
