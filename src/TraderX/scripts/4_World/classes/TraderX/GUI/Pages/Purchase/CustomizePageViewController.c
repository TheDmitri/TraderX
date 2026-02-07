class CustomizePageViewController: ViewController
{
    ref ObservableCollection<ref CategoryCardView> category_card_list = new ObservableCollection<ref CategoryCardView>(this);
    ref ObservableCollection<ref VariantCardView> variant_list = new ObservableCollection<ref VariantCardView>(this);
    ref ObservableCollection<ref ScriptView> preset_list = new ObservableCollection<ref ScriptView>(this);

    ref ObservableCollection<ref ScriptView> modal_parent = new ObservableCollection<ref ScriptView>(this);

    // Legacy fields - keeping for compatibility during transition
    ref array<ref TraderXProduct> itemAttachments;
    ref TraderXCategory attachmentsCategory;
    ref TraderXPreset clickedActionPreset;
    ref TraderXProduct originalItem;
    ref TraderXProduct currentItem;
    EntityAI preview;

    static ref CustomizePageViewController instance;

    // UI widgets
    ItemPreviewWidget itemPreview;
    ButtonWidget selectedBtn, presetBtn, variantsBtn, attachmentsBtn;
    Widget presetGrid, variantsGrid, attachmentsGrid, modalPanel, previewBtn;
    
    // Data-bound properties (auto-bind to UI via DabsFramework)
    string product_description, total_amount, money_amount, product_classname, base_price, preset_name, variant_name;

    private float sizeX;
    private float sizeY;

    private   int               itemPreviewRotX;
    private   int               itemPreviewRotY;
    private   vector            itemPreviewOrientation;
    private   int 				itemPreviewScale;

    static ref CustomizePageViewController GetInstance()
    {
        return instance;
    }
    
    void CustomizePageViewController()
    {
        itemAttachments = new array<ref TraderXProduct>();
        TraderXTradingService.GetInstance().SetMaxQuantity(false);
    }
    
    void ~CustomizePageViewController()
    {
        UnregisterEventHandlers();
        UnregisterServiceEvents();
        
        if (preview)
        {
            GetGame().ObjectDelete(preview);
            preview = null;
        }
        
        if (instance == this)
            instance = null;
    }
    
    void UnregisterEventHandlers()
    {
        ItemCardViewController.Event_OnItemCardClickEventCallBack.Remove(OnItemCardSelected);
        NewPresetCardViewController.Event_OnNewPresetClickCallBack.Remove(OnNewPresetClick);
        
        VariantCardViewController.Event_OnVariantClickCallBack.Remove(OnVariantClick);
        ModalCreatePresetViewController.Event_OnConfirmClickCallBack.Remove(OnConfirmClick);
        ModalCreatePresetViewController.Event_OnCancelClickCallBack.Remove(OnCancelClick);
        ModalConfirmationViewController.Event_OnConfirmClickCallBack.Remove(OnConfirmDeleteClick);
        ModalConfirmationViewController.Event_OnCancelClickCallBack.Remove(OnCancelDeleteClick);

        PresetCardViewController.Event_OnPresetClickCallBack.Remove(OnPresetClick);
        PresetCardViewController.Event_OnDefaultPresetClickCallBack.Remove(OnDefaultPresetClick);
        PresetCardViewController.Event_OnEditPresetClickCallBack.Remove(OnEditPresetClick);
        PresetCardViewController.Event_OnDeletePresetClickCallBack.Remove(OnDeletePresetClick);
    }
    
    void UnregisterServiceEvents()
    {
        CustomizePreviewService.Event_OnPreviewUpdated.Remove(OnPreviewUpdated);
        CustomizePreviewService.Event_OnAttachmentsChanged.Remove(OnAttachmentsChanged);
        CustomizeStateService.Event_OnVariantChanged.Remove(OnVariantChanged);
        CustomizeStateService.Event_OnPresetChanged.Remove(OnPresetChanged);
        CustomizeStateService.Event_OnConfigurationChanged.Remove(OnConfigurationChanged);
    }
    
    void RegisterEventHandlers()
    {
        // Remove first to prevent duplicate registrations from previous instances
        UnregisterEventHandlers();
        
        ItemCardViewController.Event_OnItemCardClickEventCallBack.Insert(OnItemCardSelected);
        NewPresetCardViewController.Event_OnNewPresetClickCallBack.Insert(OnNewPresetClick);
        
        VariantCardViewController.Event_OnVariantClickCallBack.Insert(OnVariantClick);
        ModalCreatePresetViewController.Event_OnConfirmClickCallBack.Insert(OnConfirmClick);
        ModalCreatePresetViewController.Event_OnCancelClickCallBack.Insert(OnCancelClick);
        ModalConfirmationViewController.Event_OnConfirmClickCallBack.Insert(OnConfirmDeleteClick);
        ModalConfirmationViewController.Event_OnCancelClickCallBack.Insert(OnCancelDeleteClick);

        PresetCardViewController.Event_OnPresetClickCallBack.Insert(OnPresetClick);
        PresetCardViewController.Event_OnDefaultPresetClickCallBack.Insert(OnDefaultPresetClick);
        PresetCardViewController.Event_OnEditPresetClickCallBack.Insert(OnEditPresetClick);
        PresetCardViewController.Event_OnDeletePresetClickCallBack.Insert(OnDeletePresetClick);
    }
    
    void RegisterServiceEvents()
    {
        // Remove first to prevent duplicate registrations from previous instances
        UnregisterServiceEvents();
        
        CustomizePreviewService.Event_OnPreviewUpdated.Insert(OnPreviewUpdated);
        CustomizePreviewService.Event_OnAttachmentsChanged.Insert(OnAttachmentsChanged);
        CustomizeStateService.Event_OnVariantChanged.Insert(OnVariantChanged);
        CustomizeStateService.Event_OnPresetChanged.Insert(OnPresetChanged);
        CustomizeStateService.Event_OnConfigurationChanged.Insert(OnConfigurationChanged);
    }

    void Setup(TraderXProduct item)
    {
        instance = this;
        
        // Register events AFTER setting instance so old instance destructor
        // (triggered by losing its static ref) runs first and cleans up its handlers
        RegisterEventHandlers();
        RegisterServiceEvents();
        
        TraderXUINavigationService.GetInstance().SetNavigationId(ENavigationIds.CUSTOMIZE);
        TraderXTradingService.GetInstance().SetTradeMode(ETraderXTradeMode.BUY);
        originalItem = item;
        currentItem = item;

        originalItem.defaultPreset = null;
        
        // Initialize services
        CustomizeStateService.GetInstance().Initialize(item);
        
        // Select the initial item
        TraderXSelectionService.GetInstance().SelectItem(item);
        
        // Update UI properties
        product_classname = item.GetDisplayName();
        product_description = CustomizePreviewService.GetInstance().GetItemDescription();
        base_price = TraderXQuantityManager.GetFormattedMoneyAmount(item.buyPrice);
        
        
        FillAttachmentList();
        UpdatePlayerMoney();
        
        // Auto-select default preset if one exists
        SelectDefaultPresetIfExists();

        NotifyPropertiesChanged({"product_classname", "product_description", "base_price"});
    }
    
    void SelectDefaultPresetIfExists()
    {
        // Always check presets under the original item's product ID (where they are stored)
        string storageProductId = originalItem.productId;
        
        // Check for user default preset first (higher priority - user preference)
        TraderXPreset defaultPreset = TraderXPresetsService.GetInstance().GetDefaultPreset(storageProductId);
        
        // If no user default, fallback to server default preset
        if (!defaultPreset)
        {
            defaultPreset = TraderXPresetsService.GetInstance().GetServerDefaultPreset(storageProductId);
        }
        
        if (defaultPreset)
        {
            // Clear current selections and apply preset to checkout (same as OnPresetClick)
            TraderXSelectionService.GetInstance().DeSelectAllItems();
            
            // Select main item first
            TraderXSelectionService.GetInstance().SelectItem(originalItem);
            
            // Apply preset attachments with proper multiplier counting
            map<string, int> attachmentCounts = new map<string, int>();
            
            // Count occurrences of each attachment
            foreach (string attachmentId : defaultPreset.attachments)
            {
                if (attachmentCounts.Contains(attachmentId))
                {
                    attachmentCounts.Set(attachmentId, attachmentCounts.Get(attachmentId) + 1);
                }
                else
                {
                    attachmentCounts.Set(attachmentId, 1);
                }
            }
            
            // Select each unique attachment with proper multiplier
            foreach (string attachId, int count : attachmentCounts)
            {
                TraderXProduct attachment = TraderXProductRepository.GetItemById(attachId);
                if (attachment)
                {
                    attachment.SetMultiplier(count);
                    TraderXSelectionService.GetInstance().SelectItem(attachment);
                }
            }
            
            // Set preset on main item for checkout
            originalItem.defaultPreset = defaultPreset;
            
            // Set current preset in state service to update preview
            CustomizeStateService.GetInstance().SetCurrentPreset(defaultPreset); 
        }
        else
        {
            GetTraderXLogger().LogDebug("SelectDefaultPresetIfExists - No default preset found");
        }
    }

    TraderXProduct GetItem()
    {
        return currentItem;
    }

    string GetItemDescription()
    {
        return CustomizePreviewService.GetInstance().GetItemDescription();
    }

    void SetItemCard(TraderXProduct item)
    {
        // Deselect previous item if exists
        if(currentItem)
        {
            TraderXSelectionService.GetInstance().DeselectItem(currentItem);
        }
        
        // Select new item
        TraderXSelectionService.GetInstance().SelectItem(item);
        
        // Update current item reference
        currentItem = item;
        
        // Delegate to preview service
        preview = CustomizePreviewService.GetInstance().CreatePreview(item);
        NotifyPropertyChanged("preview");
    }

    void FillAttachmentList()
    {
        category_card_list.Clear();

        attachmentsCategory = TraderXCategory.CreateCategory("Attachments", ETraderXCategoryType.ATTACHMENTS);

        array<ref TraderXProduct> attachments = originalItem.GetAttachments();
        foreach(TraderXProduct attachment : attachments)
        {
            attachmentsCategory.productIds.Insert(attachment.productId);
        }

        category_card_list.Insert(new CategoryCardLargeView(attachmentsCategory, EItemCardSize.LARGE, true, ETraderXCategoryType.ATTACHMENTS));
    }

    void OnItemCardSelected(ItemCardViewController itemSelected, int command = ETraderXClickEvents.LCLICK)
    {
        switch(command)
        {
            case ETraderXClickEvents.LCLICK:
                OnItemCardLClick(itemSelected);
            break;

            default:
                GetTraderXLogger().LogWarning("Unknown item card click event");
                break;
        }
    }

    void ConfigurePreset(TraderXPreset preset)
    {
        preset_name = preset.presetName;
        NotifyPropertiesChanged({"preset_name"});

        if(preset.productId != originalItem.productId)
        {
            TraderXProduct variantItem = TraderXProductRepository.GetItemById(preset.productId);
            if(!variantItem)
                return;

            SetItemCard(variantItem);  
        }
            
        foreach(string productId: preset.attachments)
        {
            TraderXProduct attachment = TraderXProductRepository.GetItemById(productId);
            if(!attachment)
                continue;

            EntityAI entAttach = EntityAI.Cast(GetGame().CreateObjectEx(attachment.className, vector.Zero, ECE_LOCAL|ECE_NOLIFETIME));
            if(!entAttach)
                continue;

            AttachToPreview(entAttach, attachment);
        }
    }

    void FillPresetList()
    {
        preset_list.Clear();
        
        // Add server presets first (admin-configured)
        TraderXPresets serverPresets = TraderXPresetsService.GetInstance().GetServerPresets(originalItem.productId);
        if (serverPresets && serverPresets.presets)
        {
            foreach (TraderXPreset serverPreset : serverPresets.presets)
            {
                preset_list.Insert(new PresetCardView(serverPreset, true)); // true indicates server preset
            }
        }
        
        // Add user presets (user-created)
        array<ref TraderXPreset> userPresets = TraderXPresetsService.GetInstance().GetPresets(originalItem.productId);
        foreach (TraderXPreset userPreset : userPresets)
        {
            preset_list.Insert(new PresetCardView(userPreset, false)); // false indicates user preset
        }

        // Add "New Preset" card if there's room (only for user presets)
        if(preset_list.Count() < 19){
            preset_list.Insert(new NewPresetCardView());
        }
    }

    void FillVariantList()
    {
        array<ref TraderXProduct> variants = originalItem.GetVariants();
        variant_list.Clear();
        foreach (TraderXProduct variant: variants)
        {
            variant_list.Insert(new VariantCardView(variant));
        }
    }

    bool IsItemAlreadyAnAttachment(TraderXProduct tpItem)
    {
        foreach(TraderXProduct attachment: itemAttachments)
        {
            if(tpItem.productId == attachment.productId)
                return true;
        }

        return false;
    }

    void RemoveAttachmentIfExist(EntityAI entityAI, TraderXProduct tpItem)
    {
        array<EntityAI> entAttachments = new array<EntityAI>();
        TraderXInventoryManager.GetEntitiesChildren(preview, entAttachments);

        for(int i = entAttachments.Count() - 1; i >= 0; i--)
        {
            if(CF_String.EqualsIgnoreCase(entAttachments[i].GetType(), tpItem.className))
            {
                GetGame().ObjectDelete(entAttachments[i]);
                itemAttachments.Remove(i);
            }
        }
        UpdateAttachmentList();
    }

    void AttachToPreview(EntityAI entityAI, TraderXProduct tpItem)
    {
        bool hasBeenAdded = TraderXAttachmentHandler.TryAttachItem(preview, entityAI);
        if(hasBeenAdded)
            itemAttachments.Insert(tpItem);

        UpdateAttachmentList();
        NotifyPropertyChanged("preview");
    }

    void OnItemCardLClick(ItemCardViewController itemSelected)
    {
        if(itemSelected.categoryType == ETraderXCategoryType.ATTACHMENTS)
        {
            TraderXProduct attachment = itemSelected.GetItem();
            if(CustomizePreviewService.GetInstance().HasAttachment(attachment)){
                CustomizePreviewService.GetInstance().RemoveAttachment(attachment);
            }
            else{
                CustomizePreviewService.GetInstance().AddAttachment(attachment);
            }
            return;
        }
    }

    void SetVariant(TraderXProduct item)
    {
        // Delegate to state service to handle variant change
        CustomizeStateService.GetInstance().SetCurrentVariant(item);
        
        // The OnVariantChanged event handler will update the UI
    }

    void UpdateAttachmentList()
    {
        // Get current items from checkout instead of preview
        array<ref CheckoutCardView> checkoutItems = CheckoutViewController.GetInstance().GetCheckoutItems();
        
        // Update UI selection states for all attachment cards
        foreach(ItemCardView itemCardView : category_card_list.Get(0).GetTemplateController().item_card_list.GetArray())
        {
            if(!itemCardView)
                continue;

            bool isSelected = false;
            TraderXProduct cardItem = itemCardView.GetTemplateController().GetItem();
            
            // Check if this card's item is in checkout list
            foreach(CheckoutCardView checkoutCard : checkoutItems)
            {
                if(!checkoutCard)
                    continue;
                    
                TraderXProduct checkoutItem = checkoutCard.GetTemplateController().GetItem();
                if(checkoutItem.productId == cardItem.productId)
                {
                    isSelected = true;
                    break;
                }
            }
            
            // Update card selection state
            itemCardView.GetTemplateController().SetItemSelected(isSelected);
        }

        // Update legacy itemAttachments for compatibility with checkout items
        itemAttachments.Clear();
        foreach(CheckoutCardView checkotCard : checkoutItems)
        {
            if(!checkotCard)
                continue;
                
            TraderXProduct checkotItem = checkotCard.GetTemplateController().GetItem();
            // Skip the main item, only include attachments/extras
            if(checkotItem.productId != originalItem.productId)
            {
                itemAttachments.Insert(checkotItem);
            }
        }

        // Update original item with selected attachments from checkout
        array<string> attachmentIds = new array<string>();
        foreach(TraderXProduct attachment : itemAttachments)
        {
            attachmentIds.Insert(attachment.productId);
        }
        
        if(originalItem)
            originalItem.SetSelectedAttachmentIds(attachmentIds);
    }

    // Navigation buttons
    bool OnPresetNavExecute(ButtonCommandArgs args)
    {
        presetGrid.Show(true);
        variantsGrid.Show(false);
        attachmentsGrid.Show(false);

        FillPresetList();
        UpdatePlayerMoney();
        
        return true;
    }

    bool OnVariantNavExecute(ButtonCommandArgs args)
    {
        presetGrid.Show(false);
        variantsGrid.Show(true);
        attachmentsGrid.Show(false);

        FillVariantList();
        UpdatePlayerMoney();
        
        return true;
    }

    bool OnAttachNavExecute(ButtonCommandArgs args)
    {
        presetGrid.Show(false);
        variantsGrid.Show(false);
        attachmentsGrid.Show(true);

        FillAttachmentList();
        UpdatePlayerMoney();
        
        return true;
    }

    void OnNewPresetClick()
    {
        modalPanel.Show(true);
        modal_parent.Clear();
        modal_parent.Insert(new ModalCreatePresetView());
    }

    void OnVariantClick(TraderXProduct item)
    {
        CustomizeStateService.GetInstance().SetCurrentVariant(item);
    }

    void OnPresetClick(TraderXPreset preset)
    {
        // Clear current selections and apply preset to checkout
        TraderXSelectionService.GetInstance().DeSelectAllItems();
        
        // Select main item first
        TraderXSelectionService.GetInstance().SelectItem(originalItem);
        
        // Apply preset attachments with proper multiplier counting
        map<string, int> attachmentCounts = new map<string, int>();
        
        // Count occurrences of each attachment
        foreach (string attachmentId : preset.attachments)
        {
            if (attachmentCounts.Contains(attachmentId))
            {
                attachmentCounts.Set(attachmentId, attachmentCounts.Get(attachmentId) + 1);
            }
            else
            {
                attachmentCounts.Set(attachmentId, 1);
            }
        }
        
        // Select each unique attachment with proper multiplier
        foreach (string attachmenId, int count : attachmentCounts)
        {
            TraderXProduct attachment = TraderXProductRepository.GetItemById(attachmenId);
            if (attachment)
            {
                attachment.SetMultiplier(count);
                TraderXSelectionService.GetInstance().SelectItem(attachment);
            }
        }
        
        // Set preset on main item for checkout
        originalItem.defaultPreset = preset;
        
        CustomizeStateService.GetInstance().SetCurrentPreset(preset);
    }

    void OnCancelClick()
    {
        modal_parent.Clear();
        modalPanel.Show(false);
    }

    // Modal Create Preset
    void OnConfirmClick(string name)
    {
        bool success = CustomizeStateService.GetInstance().CreatePreset(name);
        
        modal_parent.Clear();
        modalPanel.Show(false);

        if (success)
        {
            FillPresetList();
        }
    }

    void UpdatePlayerMoney()
    {
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if(!player)
            return;
    
        int playerMoney = TraderXTradingService.GetInstance().GetPlayerMoneyAmount(player);
        money_amount = TraderXQuantityManager.GetFormattedMoneyAmount(playerMoney);
        
        // Update total amount with current configuration price
        
        NotifyPropertiesChanged({"money_amount", "total_amount"});
    }

    void OnDefaultPresetClick(TraderXPreset preset)
    {
        if (!preset || !originalItem)
            return;
        
        // Check if this is a server preset - server presets can't be set as user defaults
        bool isServerPreset = IsServerPreset(preset);
        if (isServerPreset)
        {
            GetTraderXLogger().LogWarning("OnDefaultPresetClick - Cannot set server preset as user default");
            return;
        }
        
        // Always use original item's product ID for preset storage and default setting
        string storageProductId = originalItem.productId;
        
        TraderXPresetsService.GetInstance().SetDefaultPreset(storageProductId, preset.presetId);
        
        // Only refresh existing card colors — do NOT call FillPresetList here
        // because it destroys the PresetCardViewController that is still on the call stack
        if (preset_list)
            RefreshAllPresetDefaultButtons();
    }
    
    void RefreshAllPresetDefaultButtons()
    {
        if (!preset_list)
            return;
        
        array<ref ScriptView> cards = preset_list.GetArray();
        if (!cards)
            return;
        
        for (int i = 0; i < cards.Count(); i++)
        {
            if (!cards[i])
                continue;
            
            PresetCardView presetCardView = PresetCardView.Cast(cards[i]);
            if (presetCardView && presetCardView.GetTemplateController())
            {
                PresetCardViewController controller = PresetCardViewController.Cast(presetCardView.GetTemplateController());
                if (controller)
                {
                    controller.RefreshDefaultStatus(originalItem.productId);
                }
            }
        }
    }

    void OnEditPresetClick(TraderXPreset preset)
    {
        // Check if this is a server preset - server presets can't be edited by users
        bool isServerPreset = IsServerPreset(preset);
        if (isServerPreset)
        {
            GetTraderXLogger().LogWarning("OnEditPresetClick - Cannot edit server preset");
            return;
        }
        
        // TODO: Implement user preset editing functionality
    }

    void OnDeletePresetClick(TraderXPreset preset)
    {
        // Check if this is a server preset - server presets can't be deleted by users
        bool isServerPreset = IsServerPreset(preset);
        if (isServerPreset)
        {
            GetTraderXLogger().LogWarning("OnDeletePresetClick - Cannot delete server preset");
            return;
        }

        modalPanel.Show(true);
        modal_parent.Clear();
        clickedActionPreset = preset;
        modal_parent.Insert(new ModalConfirmationView("Delete preset", "Are you sure you want to delete this preset?"));
    }

    void OnConfirmDeleteClick()
    {
        modalPanel.Show(false);
        if(!clickedActionPreset)
            return;
        
        TraderXPresetsService.GetInstance().RemovePreset(originalItem.productId, clickedActionPreset.presetId);
        clickedActionPreset = null;
        FillPresetList();
    }

    void OnCancelDeleteClick()
    {
        modalPanel.Show(false);
        clickedActionPreset = null;
    }
    
    // === SERVER PRESET HELPER METHODS ===
    
    bool IsServerPreset(TraderXPreset preset)
    {
        if (!preset)
            return false;
            
        if (!originalItem)
            return false;
        
        TraderXPresets serverPresets = TraderXPresetsService.GetInstance().GetServerPresets(originalItem.productId);
        if (!serverPresets || !serverPresets.presets)
            return false;
            
        foreach (TraderXPreset serverPreset : serverPresets.presets)
        {
            if (serverPreset.presetId == preset.presetId)
                return true;
        }
        
        return false;
    }

    // === SERVICE EVENT HANDLERS ===
    
    void OnPreviewUpdated(EntityAI newPreview)
    {
        preview = newPreview;
        NotifyPropertyChanged("preview");
    }
    
    void OnAttachmentsChanged(array<ref TraderXProduct> attachments)
    {
        // Update UI attachment selection states
        UpdateAttachmentList();
        UpdatePlayerMoney(); // Update total price
    }
    
    void OnVariantChanged(TraderXProduct variant)
    {
        currentItem = variant;
        variant_name = CustomizeStateService.GetInstance().GetCurrentVariantName();
        product_description = GetItemDescription();
        NotifyPropertiesChanged({"variant_name", "product_description"});
        UpdatePlayerMoney();
        
        // Auto-select default preset for the new variant if one exists
        SelectDefaultPresetIfExists();
    }
    
    void OnPresetChanged(TraderXPreset preset)
    {
        if(preset)
        {
            preset_name = CustomizeStateService.GetInstance().GetCurrentPresetName();
            variant_name = CustomizeStateService.GetInstance().GetCurrentVariantName();
            NotifyPropertiesChanged({"preset_name", "variant_name"});
        }
        else
        {
            preset_name = "";
            NotifyPropertyChanged("preset_name");
        }
        
        product_description = GetItemDescription();
        NotifyPropertyChanged("product_description");
        UpdatePlayerMoney();
    }
    
    void OnConfigurationChanged(TraderXProduct configuration)
    {
        currentItem = configuration;
        UpdatePlayerMoney();
    }

    //when mousebuttondown used, start update rotation for either item preview or player preview
    override bool OnMouseButtonDown(Widget w, int x, int y, int button)
    {
        super.OnMouseButtonDown(w, x, y, button);
        if (w != previewBtn)
            return false;

        GetGame().GetDragQueue().Call(this, "UpdateRotation");
        g_Game.GetMousePos(itemPreviewRotX, itemPreviewRotY);
        return true;
    }

    //update rotation for item preview
    void UpdateRotation(int mouse_x, int mouse_y, bool is_dragging)
    {
        vector orientation = itemPreviewOrientation;
        orientation[0] = orientation[0] + (itemPreviewRotY - mouse_y);
        orientation[1] = orientation[1] - (itemPreviewRotX - mouse_x);

        itemPreview.SetModelOrientation( orientation );

        if (!is_dragging)
            itemPreviewOrientation = orientation;
    }

    //update item preview scale
  void UpdateScale()
  {
    float w, h, x, y;
    itemPreview.GetPos(x, y);
    itemPreview.GetSize(w,h);
    w = w + ( itemPreviewScale / 4);
    h = h + ( itemPreviewScale / 4 );
    if ( w > 0.5 && w < 3 )
    {
      itemPreview.SetSize( w, h );

      // align to center
      int screen_w, screen_h;
      GetScreenSize(screen_w, screen_h);
      float new_x = x - ( itemPreviewScale / 8 );
      float new_y = y - ( itemPreviewScale / 8 );
      itemPreview.SetPos( new_x, new_y );
    }
  }

    //update item preview scale, maybe player too in the future
    override bool OnMouseWheel(Widget  w, int  x, int  y, int wheel)
    {
        super.OnMouseWheel(w, x, y, wheel);
        if (w != previewBtn)
            return false;

        itemPreviewScale = wheel;
        UpdateScale();
        return true;
    }
}