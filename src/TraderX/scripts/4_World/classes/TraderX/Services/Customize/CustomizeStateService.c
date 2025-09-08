class CustomizeStateService
{
    private static ref CustomizeStateService m_Instance;
    
    // Events for UI coordination
    static ref ScriptInvoker Event_OnVariantChanged = new ScriptInvoker();
    static ref ScriptInvoker Event_OnPresetChanged = new ScriptInvoker();
    static ref ScriptInvoker Event_OnConfigurationChanged = new ScriptInvoker();
    
    // Current state
    private TraderXProduct m_OriginalItem;
    private TraderXProduct m_CurrentItem;
    private TraderXPreset m_CurrentPreset;
    private string m_CurrentVariantName;
    private string m_CurrentPresetName;
    
    void CustomizeStateService()
    {
    }
    
    static CustomizeStateService GetInstance()
    {
        if (!m_Instance)
            m_Instance = new CustomizeStateService();
        return m_Instance;
    }
    
    // === INITIALIZATION ===
    
    void Initialize(TraderXProduct originalItem)
    {
        m_OriginalItem = originalItem;
        m_CurrentItem = originalItem;
        m_CurrentPreset = null;
        m_CurrentVariantName = originalItem.GetDisplayName();
        m_CurrentPresetName = "";
        
        // Initialize preview service with the original item
        CustomizePreviewService.GetInstance().CreatePreview(originalItem);
        
        Event_OnConfigurationChanged.Invoke(m_CurrentItem);
    }
    
    void Reset()
    {
        if (m_OriginalItem)
        {
            Initialize(m_OriginalItem);
        }
    }
    
    // === VARIANT MANAGEMENT ===
    
    bool SetCurrentVariant(TraderXProduct variant)
    {
        if (!variant)
        {
            GetTraderXLogger().LogError("CustomizeStateService::SetCurrentVariant - Variant is null");
            return false;
        }
        
        TraderXSelectionService.GetInstance().DeselectItem(m_CurrentItem);
        // Update state
        m_CurrentItem = variant;
        m_CurrentVariantName = variant.GetDisplayName();
        m_CurrentPreset = null; // Clear preset when manually selecting variant
        m_CurrentPresetName = "";
        
        // For variants, we replace the main item selection, not add as attachment
        TraderXSelectionService.GetInstance().DeselectItem(m_OriginalItem);
        TraderXSelectionService.GetInstance().SelectItem(variant);
        
        // Update preview service
        bool success = CustomizePreviewService.GetInstance().SetVariant(variant);
        if (success)
        {
            Event_OnVariantChanged.Invoke(variant);
            Event_OnConfigurationChanged.Invoke(m_CurrentItem);
        }
        
        return success;
    }
    
    TraderXProduct GetCurrentVariant()
    {
        return m_CurrentItem;
    }
    
    string GetCurrentVariantName()
    {
        return m_CurrentVariantName;
    }
    
    // === PRESET MANAGEMENT ===
    
    bool SetCurrentPreset(TraderXPreset preset)
    {
        if (!preset)
        {
            GetTraderXLogger().LogError("CustomizeStateService::SetCurrentPreset - Preset is null");
            return false;
        }
        
        TraderXSelectionService.GetInstance().SelectItem(m_CurrentItem);
        // Update state
        m_CurrentPreset = preset;
        m_CurrentPresetName = preset.presetName;
        
        // If preset uses different base item, update current item
        if (preset.productId != m_CurrentItem.productId)
        {
            TraderXProduct variantItem = TraderXProductRepository.GetItemById(preset.productId);
            if (variantItem)
            {
                TraderXSelectionService.GetInstance().DeselectItem(m_CurrentItem);
                m_CurrentItem = variantItem;
                m_CurrentVariantName = variantItem.GetDisplayName();
                TraderXSelectionService.GetInstance().SelectItem(variantItem);
            }
        }
        
        // Apply preset via preview service
        bool success = CustomizePreviewService.GetInstance().ApplyPreset(preset);
        if (success)
        {
            Event_OnPresetChanged.Invoke(preset);
            Event_OnConfigurationChanged.Invoke(m_CurrentItem);
        }
        
        return success;
    }
    
    void ClearCurrentPreset()
    {
        m_CurrentPreset = null;
        m_CurrentPresetName = "";
        
        Event_OnPresetChanged.Invoke(null);
    }
    
    TraderXPreset GetCurrentPreset()
    {
        return m_CurrentPreset;
    }
    
    string GetCurrentPresetName()
    {
        return m_CurrentPresetName;
    }
    
    bool HasActivePreset()
    {
        return m_CurrentPreset != null;
    }
    
    // === CONFIGURATION MANAGEMENT ===
    
    TraderXProduct GetCurrentConfiguration()
    {
        // Create a copy of current item with selected attachments
        TraderXProduct configuration = m_CurrentItem;
        
        // Get selected attachments from checkout instead of preview service
        array<ref CheckoutCardView> checkoutItems = CheckoutViewController.GetInstance().GetCheckoutItems();
        array<string> attachmentIds = new array<string>();
        
        foreach (CheckoutCardView checkoutCard : checkoutItems)
        {
            if (!checkoutCard)
                continue;
                
            TraderXProduct checkoutItem = checkoutCard.GetTemplateController().GetItem();
            // Skip the main item and current variant, only include attachments/extras
            if (checkoutItem.productId != m_OriginalItem.productId && checkoutItem.productId != m_CurrentItem.productId)
            {
                // Add each attachment based on its multiplier
                for (int i = 0; i < checkoutItem.GetMultiplier(); i++)
                {
                    attachmentIds.Insert(checkoutItem.productId);
                }
            }
        }
        
        configuration.SetSelectedAttachmentIds(attachmentIds);
        
        return configuration;
    }
    
    TraderXProduct GetOriginalItem()
    {
        return m_OriginalItem;
    }
    
    // === VALIDATION ===
    
    bool IsConfigurationValid()
    {
        // Check if current configuration is valid
        if (!m_CurrentItem)
            return false;
            
        // Additional validation logic can be added here
        // For example: check if all attachments are compatible
        
        return true;
    }
    
    bool HasChangesFromOriginal()
    {
        if (!m_OriginalItem || !m_CurrentItem)
            return false;
            
        // Check if variant changed
        if (m_CurrentItem.productId != m_OriginalItem.productId)
            return true;
            
        // Check if attachments changed
        array<ref TraderXProduct> currentAttachments = CustomizePreviewService.GetInstance().GetCurrentAttachments();
        array<ref TraderXProduct> originalAttachments = m_OriginalItem.GetAttachments();
        
        if (currentAttachments.Count() != originalAttachments.Count())
            return true;
            
        // Compare attachment lists (simplified comparison)
        foreach (TraderXProduct currentAttachment : currentAttachments)
        {
            bool found = false;
            foreach (TraderXProduct originalAttachment : originalAttachments)
            {
                if (currentAttachment.productId == originalAttachment.productId)
                {
                    found = true;
                    break;
                }
            }
            if (!found)
                return true;
        }
        
        return false;
    }
    
    // === PRESET CREATION SUPPORT ===
    
    bool CreatePreset(string presetName)
    {
        if (presetName == "")
        {
            GetTraderXLogger().LogError("CustomizeStateService::CreatePreset - Invalid preset name");
            return false;
        }
        
        if (!m_CurrentItem)
        {
            GetTraderXLogger().LogError("CustomizeStateService::CreatePreset - No current item");
            return false;
        }
        
        if (!m_OriginalItem)
        {
            GetTraderXLogger().LogError("CustomizeStateService::CreatePreset - No original item");
            return false;
        }
        
        TraderXProduct configuration = GetCurrentConfiguration();

        array<string> attachments = configuration.GetSelectedAttachments();
        
        // Create preset via service - store under original item but preset contains current variant
        TraderXPresetsService.GetInstance().AddPreset(m_OriginalItem.productId, presetName, attachments, m_CurrentItem.productId);        
        return true;
    }
    
    // === UTILITY METHODS ===
    
    void LogCurrentState()
    {
        GetTraderXLogger().LogDebug("=== CustomizeStateService Current State ===");
        GetTraderXLogger().LogDebug("Original Item: " + Ternary<string>.If(m_OriginalItem != null , m_OriginalItem.GetDisplayName(), "null"));
        GetTraderXLogger().LogDebug("Current Item: " + Ternary<string>.If(m_CurrentItem != null, m_CurrentItem.GetDisplayName(), "null"));
        GetTraderXLogger().LogDebug("Current Variant Name: " + m_CurrentVariantName);
        GetTraderXLogger().LogDebug("Current Preset: " + Ternary<string>.If(m_CurrentPreset != null, m_CurrentPreset.presetName, "none"));
        GetTraderXLogger().LogDebug("Current Preset Name: " + m_CurrentPresetName);
        GetTraderXLogger().LogDebug("Has Changes: " + HasChangesFromOriginal());
        GetTraderXLogger().LogDebug("Is Valid: " + IsConfigurationValid());
        
        array<ref TraderXProduct> attachments = CustomizePreviewService.GetInstance().GetCurrentAttachments();
        GetTraderXLogger().LogDebug("Current Attachments: " + attachments.Count());
        foreach (TraderXProduct attachment : attachments)
        {
            GetTraderXLogger().LogDebug("  - " + attachment.GetDisplayName());
        }
        GetTraderXLogger().LogDebug("=== End State ===");
    }
    
    // === PRICE CALCULATION ===
    
    int GetCurrentTotalPrice()
    {
        if (!m_CurrentItem)
            return 0;
            
        int totalPrice = m_CurrentItem.buyPrice;
        
        // Add attachment prices
        array<ref TraderXProduct> attachments = CustomizePreviewService.GetInstance().GetCurrentAttachments();
        foreach (TraderXProduct attachment : attachments)
        {
            totalPrice += attachment.buyPrice;
        }
        
        return totalPrice;
    }
    
    string GetFormattedTotalPrice()
    {
        return TraderXQuantityManager.GetFormattedMoneyAmount(GetCurrentTotalPrice());
    }
}
