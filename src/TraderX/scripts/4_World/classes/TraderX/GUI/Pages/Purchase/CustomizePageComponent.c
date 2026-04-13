class CustomizePageComponent extends DZlrComponentBase
{
    private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("traderx-customize-page", CustomizePageComponent);

    ref array<ref CategoryCardComponent> m_CategoryCardList = new array<ref CategoryCardComponent>();
    ref array<ref VariantCardComponent> m_VariantList = new array<ref VariantCardComponent>();
    ref array<ref DZlrComponentBase> m_PresetList = new array<ref DZlrComponentBase>();

    ref array<ref TraderXProduct> itemAttachments;
    ref TraderXCategory attachmentsCategory;
    ref TraderXPreset clickedActionPreset;
    ref TraderXProduct originalItem;
    ref TraderXProduct currentItem;
    EntityAI preview;

    static ref CustomizePageComponent instance;

    // Widgets
    ItemPreviewWidget itemPreview;
    ButtonWidget selectedBtn, presetBtn, variantsBtn, attachmentsBtn, closeBtn;
    Widget presetGrid, variantsGrid, attachmentsGrid, modalPanel, previewBtn;
    Widget category_card_list, variant_list, preset_list, modal_parent;
    TextWidget productDescriptionText, totalAmountText, moneyAmountText, productClassnameText, basePriceText, presetNameText, variantNameText;

    // Data-bound properties
    string product_description, total_amount, money_amount, product_classname, base_price, preset_name, variant_name;

    private float sizeX, sizeY;
    private int itemPreviewRotX, itemPreviewRotY;
    private vector itemPreviewOrientation;
    private int itemPreviewScale;

    static ref CustomizePageComponent GetInstance()
    {
        return instance;
    }

    void CustomizePageComponent(TraderXProduct item)
    {
        originalItem = item;
        itemAttachments = new array<ref TraderXProduct>();
        TraderXTradingService.GetInstance().SetMaxQuantity(false);
    }

    override protected void RegisterMetadata()
    {
        m_Metadata = new ComponentMetadata();
        m_Metadata.selector = "traderx-customize-page";
        m_Metadata.templatePath = "TraderX/datasets/gui/PurchasePage/CustomizePageVariant.layout";
    }

    override void OnInit()
    {
        instance = this;
        RegisterEventHandlers();
        RegisterServiceEvents();
        Setup(originalItem);
    }

    override void OnDestroy()
    {
        UnregisterEventHandlers();
        UnregisterServiceEvents();

        if (preview)
        {
            GetGame().ObjectDelete(preview);
            preview = null;
        }

        ClearCategoryCards();
        ClearVariantList();
        ClearPresetList();

        if (instance == this)
            instance = null;
    }

    void UnregisterEventHandlers()
    {
        ItemCardComponent.Event_OnItemCardClickEventCallBack.Remove(OnItemCardSelected);
        NewPresetCardComponent.Event_OnNewPresetClickCallBack.Remove(OnNewPresetClick);
        VariantCardComponent.Event_OnVariantClickCallBack.Remove(OnVariantClick);
        ModalCreatePresetComponent.Event_OnConfirmClickCallBack.Remove(OnConfirmClick);
        ModalCreatePresetComponent.Event_OnCancelClickCallBack.Remove(OnCancelClick);
        ModalConfirmationComponent.Event_OnConfirmClickCallBack.Remove(OnConfirmDeleteClick);
        ModalConfirmationComponent.Event_OnCancelClickCallBack.Remove(OnCancelDeleteClick);
        PresetCardComponent.Event_OnPresetClickCallBack.Remove(OnPresetClick);
        PresetCardComponent.Event_OnDefaultPresetClickCallBack.Remove(OnDefaultPresetClick);
        PresetCardComponent.Event_OnEditPresetClickCallBack.Remove(OnEditPresetClick);
        PresetCardComponent.Event_OnDeletePresetClickCallBack.Remove(OnDeletePresetClick);
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
        UnregisterEventHandlers();

        ItemCardComponent.Event_OnItemCardClickEventCallBack.Insert(OnItemCardSelected);
        NewPresetCardComponent.Event_OnNewPresetClickCallBack.Insert(OnNewPresetClick);
        VariantCardComponent.Event_OnVariantClickCallBack.Insert(OnVariantClick);
        ModalCreatePresetComponent.Event_OnConfirmClickCallBack.Insert(OnConfirmClick);
        ModalCreatePresetComponent.Event_OnCancelClickCallBack.Insert(OnCancelClick);
        ModalConfirmationComponent.Event_OnConfirmClickCallBack.Insert(OnConfirmDeleteClick);
        ModalConfirmationComponent.Event_OnCancelClickCallBack.Insert(OnCancelDeleteClick);
        PresetCardComponent.Event_OnPresetClickCallBack.Insert(OnPresetClick);
        PresetCardComponent.Event_OnDefaultPresetClickCallBack.Insert(OnDefaultPresetClick);
        PresetCardComponent.Event_OnEditPresetClickCallBack.Insert(OnEditPresetClick);
        PresetCardComponent.Event_OnDeletePresetClickCallBack.Insert(OnDeletePresetClick);
    }

    void RegisterServiceEvents()
    {
        UnregisterServiceEvents();

        CustomizePreviewService.Event_OnPreviewUpdated.Insert(OnPreviewUpdated);
        CustomizePreviewService.Event_OnAttachmentsChanged.Insert(OnAttachmentsChanged);
        CustomizeStateService.Event_OnVariantChanged.Insert(OnVariantChanged);
        CustomizeStateService.Event_OnPresetChanged.Insert(OnPresetChanged);
        CustomizeStateService.Event_OnConfigurationChanged.Insert(OnConfigurationChanged);
    }

    void Setup(TraderXProduct item)
    {
        TraderXUINavigationService.GetInstance().SetNavigationId(ENavigationIds.CUSTOMIZE);
        TraderXTradingService.GetInstance().SetTradeMode(ETraderXTradeMode.BUY);
        currentItem = item;

        originalItem.defaultPreset = null;

        CustomizeStateService.GetInstance().Initialize(item);
        TraderXSelectionService.GetInstance().SelectItem(item);

        product_classname = item.GetDisplayName();
        product_description = CustomizePreviewService.GetInstance().GetItemDescription();
        base_price = TraderXQuantityManager.GetFormattedMoneyAmount(item.buyPrice);

        FillAttachmentList();
        UpdatePlayerMoney();
        SelectDefaultPresetIfExists();

        UpdateTextWidgets();
    }

    void UpdateTextWidgets()
    {
        if (productClassnameText) productClassnameText.SetText(product_classname);
        if (productDescriptionText) productDescriptionText.SetText(product_description);
        if (basePriceText) basePriceText.SetText(base_price);
        if (moneyAmountText) moneyAmountText.SetText(money_amount);
        if (totalAmountText) totalAmountText.SetText(total_amount);
        if (presetNameText) presetNameText.SetText(preset_name);
        if (variantNameText) variantNameText.SetText(variant_name);
    }

    void SelectDefaultPresetIfExists()
    {
        string storageProductId = originalItem.productId;

        TraderXPreset defaultPreset = TraderXPresetsService.GetInstance().GetDefaultPreset(storageProductId);
        if (!defaultPreset)
            defaultPreset = TraderXPresetsService.GetInstance().GetServerDefaultPreset(storageProductId);

        if (defaultPreset)
        {
            TraderXSelectionService.GetInstance().DeSelectAllItems();
            TraderXSelectionService.GetInstance().SelectItem(originalItem);

            map<string, int> attachmentCounts = new map<string, int>();
            foreach (string attachmentId : defaultPreset.attachments)
            {
                if (attachmentCounts.Contains(attachmentId))
                    attachmentCounts.Set(attachmentId, attachmentCounts.Get(attachmentId) + 1);
                else
                    attachmentCounts.Set(attachmentId, 1);
            }

            foreach (string attachId, int count : attachmentCounts)
            {
                TraderXProduct attachment = TraderXProductRepository.GetItemById(attachId);
                if (attachment)
                {
                    attachment.SetMultiplier(count);
                    TraderXSelectionService.GetInstance().SelectItem(attachment);
                }
            }

            originalItem.defaultPreset = defaultPreset;
            CustomizeStateService.GetInstance().SetCurrentPreset(defaultPreset);
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
        if (currentItem)
            TraderXSelectionService.GetInstance().DeselectItem(currentItem);

        TraderXSelectionService.GetInstance().SelectItem(item);
        currentItem = item;

        preview = CustomizePreviewService.GetInstance().CreatePreview(item);
        if (itemPreview) itemPreview.SetItem(preview);
    }

    void ClearCategoryCards()
    {
        foreach (CategoryCardComponent card : m_CategoryCardList)
        {
            if (card && card.GetRoot()) card.GetRoot().Unlink();
        }
        m_CategoryCardList.Clear();
    }

    void ClearVariantList()
    {
        foreach (VariantCardComponent card : m_VariantList)
        {
            if (card && card.GetRoot()) card.GetRoot().Unlink();
        }
        m_VariantList.Clear();
    }

    void ClearPresetList()
    {
        foreach (DZlrComponentBase card : m_PresetList)
        {
            if (card && card.GetRoot()) card.GetRoot().Unlink();
        }
        m_PresetList.Clear();
    }

    void FillAttachmentList()
    {
        ClearCategoryCards();

        attachmentsCategory = TraderXCategory.CreateCategory("Attachments", ETraderXCategoryType.ATTACHMENTS);

        array<ref TraderXProduct> attachments = originalItem.GetAttachments();
        foreach (TraderXProduct attachment : attachments)
        {
            attachmentsCategory.productIds.Insert(attachment.productId);
        }

        CategoryCardComponent card = new CategoryCardComponent();
        card.InitComponent(null);
        card.SetCategoryCardData(attachmentsCategory, EItemCardSize.LARGE, true, ETraderXCategoryType.ATTACHMENTS);
        if (attachmentsGrid)
        {
            attachmentsGrid.AddChild(card.GetRoot());
            card.GetRoot().Show(true);
        }
        m_CategoryCardList.Insert(card);
    }

    void FillPresetList()
    {
        ClearPresetList();

        TraderXPresets serverPresets = TraderXPresetsService.GetInstance().GetServerPresets(originalItem.productId);
        if (serverPresets && serverPresets.presets)
        {
            foreach (TraderXPreset serverPreset : serverPresets.presets)
            {
                PresetCardComponent card = new PresetCardComponent(serverPreset, true);
                card.InitComponent(null);
                if (preset_list)
                {
                    preset_list.AddChild(card.GetRoot());
                    card.GetRoot().Show(true);
                }
                m_PresetList.Insert(card);
            }
        }

        array<ref TraderXPreset> userPresets = TraderXPresetsService.GetInstance().GetPresets(originalItem.productId);
        foreach (TraderXPreset userPreset : userPresets)
        {
            PresetCardComponent card = new PresetCardComponent(userPreset, false);
            card.InitComponent(null);
            if (preset_list)
            {
                preset_list.AddChild(card.GetRoot());
                card.GetRoot().Show(true);
            }
            m_PresetList.Insert(card);
        }

        if (m_PresetList.Count() < 19)
        {
            NewPresetCardComponent newCard = new NewPresetCardComponent();
            newCard.InitComponent(null);
            if (preset_list)
            {
                preset_list.AddChild(newCard.GetRoot());
                newCard.GetRoot().Show(true);
            }
            m_PresetList.Insert(newCard);
        }
    }

    void FillVariantList()
    {
        ClearVariantList();
        array<ref TraderXProduct> variants = originalItem.GetVariants();
        foreach (TraderXProduct variant : variants)
        {
            VariantCardComponent card = new VariantCardComponent(variant);
            card.InitComponent(null);
            if (variant_list)
            {
                variant_list.AddChild(card.GetRoot());
                card.GetRoot().Show(true);
            }
            m_VariantList.Insert(card);
        }
    }

    void OnItemCardSelected(ItemCardComponent itemSelected, int command = ETraderXClickEvents.LCLICK)
    {
        if (command == ETraderXClickEvents.LCLICK)
            OnItemCardLClick(itemSelected);
    }

    void OnItemCardLClick(ItemCardComponent itemSelected)
    {
        if (itemSelected.categoryType == ETraderXCategoryType.ATTACHMENTS)
        {
            TraderXProduct attachment = itemSelected.GetItem();
            if (CustomizePreviewService.GetInstance().HasAttachment(attachment))
                CustomizePreviewService.GetInstance().RemoveAttachment(attachment);
            else
                CustomizePreviewService.GetInstance().AddAttachment(attachment);
        }
    }

    void AttachToPreview(EntityAI entityAI, TraderXProduct tpItem)
    {
        bool hasBeenAdded = TraderXAttachmentHandler.TryAttachItem(preview, entityAI);
        if (hasBeenAdded)
            itemAttachments.Insert(tpItem);

        UpdateAttachmentList();
        if (itemPreview) itemPreview.SetItem(preview);
    }

    void UpdateAttachmentList()
    {
        array<ref CheckoutCardComponent> checkoutItems = CheckoutComponent.GetInstance().GetCheckoutItems();

        foreach (CategoryCardComponent catCard : m_CategoryCardList)
        {
            array<ref ItemCardComponent> itemCards = catCard.GetItemCards();
            foreach (ItemCardComponent itemCardComp : itemCards)
            {
                bool isSelected = false;
                TraderXProduct cardItem = itemCardComp.GetItem();

                foreach (CheckoutCardComponent checkoutCard : checkoutItems)
                {
                    TraderXProduct checkoutItem = checkoutCard.GetItem();
                    if (checkoutItem.productId == cardItem.productId)
                    {
                        isSelected = true;
                        break;
                    }
                }
                itemCardComp.SetItemSelected(isSelected);
            }
        }

        itemAttachments.Clear();
        foreach (CheckoutCardComponent chkCard : checkoutItems)
        {
            TraderXProduct chkItem = chkCard.GetItem();
            if (chkItem.productId != originalItem.productId)
                itemAttachments.Insert(chkItem);
        }

        array<string> attachmentIds = new array<string>();
        foreach (TraderXProduct att : itemAttachments)
        {
            attachmentIds.Insert(att.productId);
        }

        if (originalItem)
            originalItem.SetSelectedAttachmentIds(attachmentIds);
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (w == closeBtn)
        {
            TraderXMainComponent.GetInstance().RemoveSubPage();
            return true;
        }
        if (w == presetBtn)
        {
            if (presetGrid) presetGrid.Show(true);
            if (variantsGrid) variantsGrid.Show(false);
            if (attachmentsGrid) attachmentsGrid.Show(false);
            FillPresetList();
            UpdatePlayerMoney();
            return true;
        }
        if (w == variantsBtn)
        {
            if (presetGrid) presetGrid.Show(false);
            if (variantsGrid) variantsGrid.Show(true);
            if (attachmentsGrid) attachmentsGrid.Show(false);
            FillVariantList();
            UpdatePlayerMoney();
            return true;
        }
        if (w == attachmentsBtn)
        {
            if (presetGrid) presetGrid.Show(false);
            if (variantsGrid) variantsGrid.Show(false);
            if (attachmentsGrid) attachmentsGrid.Show(true);
            FillAttachmentList();
            UpdatePlayerMoney();
            return true;
        }
        return false;
    }

    void OnNewPresetClick()
    {
        if (modalPanel) modalPanel.Show(true);
        ClearModalParent();
        ModalCreatePresetComponent modal = new ModalCreatePresetComponent();
        modal.InitComponent(null);
        if (modal_parent)
        {
            modal_parent.AddChild(modal.GetRoot());
            modal.GetRoot().Show(true);
        }
    }

    void ClearModalParent()
    {
        if (modal_parent)
        {
            Widget child = modal_parent.GetChildren();
            while (child)
            {
                Widget next = child.GetSibling();
                child.Unlink();
                child = next;
            }
        }
    }

    void OnVariantClick(TraderXProduct item)
    {
        CustomizeStateService.GetInstance().SetCurrentVariant(item);
    }

    void OnPresetClick(TraderXPreset preset)
    {
        TraderXSelectionService.GetInstance().DeSelectAllItems();
        TraderXSelectionService.GetInstance().SelectItem(originalItem);

        map<string, int> attachmentCounts = new map<string, int>();
        foreach (string attachmentId : preset.attachments)
        {
            if (attachmentCounts.Contains(attachmentId))
                attachmentCounts.Set(attachmentId, attachmentCounts.Get(attachmentId) + 1);
            else
                attachmentCounts.Set(attachmentId, 1);
        }

        foreach (string attachmenId, int count : attachmentCounts)
        {
            TraderXProduct attachment = TraderXProductRepository.GetItemById(attachmenId);
            if (attachment)
            {
                attachment.SetMultiplier(count);
                TraderXSelectionService.GetInstance().SelectItem(attachment);
            }
        }

        originalItem.defaultPreset = preset;
        CustomizeStateService.GetInstance().SetCurrentPreset(preset);
    }

    void OnCancelClick()
    {
        ClearModalParent();
        if (modalPanel) modalPanel.Show(false);
    }

    void OnConfirmClick(string name)
    {
        bool success = CustomizeStateService.GetInstance().CreatePreset(name);
        ClearModalParent();
        if (modalPanel) modalPanel.Show(false);

        if (success)
            FillPresetList();
    }

    void UpdatePlayerMoney()
    {
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player)
            return;

        int playerMoney = TraderXTradingService.GetInstance().GetPlayerMoneyAmount(player);
        money_amount = TraderXQuantityManager.GetFormattedMoneyAmount(playerMoney);

        if (moneyAmountText) moneyAmountText.SetText(money_amount);
        if (totalAmountText) totalAmountText.SetText(total_amount);
    }

    void OnDefaultPresetClick(TraderXPreset preset)
    {
        if (!preset || !originalItem)
            return;

        if (IsServerPreset(preset))
            return;

        string storageProductId = originalItem.productId;
        TraderXPresetsService.GetInstance().SetDefaultPreset(storageProductId, preset.presetId);
        RefreshAllPresetDefaultButtons();
    }

    void RefreshAllPresetDefaultButtons()
    {
        foreach (DZlrComponentBase comp : m_PresetList)
        {
            PresetCardComponent presetCard = PresetCardComponent.Cast(comp);
            if (presetCard)
                presetCard.RefreshDefaultStatus(originalItem.productId);
        }
    }

    void OnEditPresetClick(TraderXPreset preset)
    {
        if (IsServerPreset(preset))
            return;
    }

    void OnDeletePresetClick(TraderXPreset preset)
    {
        if (IsServerPreset(preset))
            return;

        if (modalPanel) modalPanel.Show(true);
        ClearModalParent();
        clickedActionPreset = preset;
        ModalConfirmationComponent modal = new ModalConfirmationComponent("Delete preset", "Are you sure you want to delete this preset?");
        modal.InitComponent(null);
        if (modal_parent)
        {
            modal_parent.AddChild(modal.GetRoot());
            modal.GetRoot().Show(true);
        }
    }

    void OnConfirmDeleteClick()
    {
        if (modalPanel) modalPanel.Show(false);
        if (!clickedActionPreset)
            return;

        TraderXPresetsService.GetInstance().RemovePreset(originalItem.productId, clickedActionPreset.presetId);
        clickedActionPreset = null;
        FillPresetList();
    }

    void OnCancelDeleteClick()
    {
        if (modalPanel) modalPanel.Show(false);
        clickedActionPreset = null;
    }

    bool IsServerPreset(TraderXPreset preset)
    {
        if (!preset || !originalItem)
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

    // Service event handlers
    void OnPreviewUpdated(EntityAI newPreview)
    {
        preview = newPreview;
        if (itemPreview) itemPreview.SetItem(preview);
    }

    void OnAttachmentsChanged(array<ref TraderXProduct> attachments)
    {
        UpdateAttachmentList();
        UpdatePlayerMoney();
    }

    void OnVariantChanged(TraderXProduct variant)
    {
        currentItem = variant;
        variant_name = CustomizeStateService.GetInstance().GetCurrentVariantName();
        product_description = GetItemDescription();
        if (variantNameText) variantNameText.SetText(variant_name);
        if (productDescriptionText) productDescriptionText.SetText(product_description);
        UpdatePlayerMoney();
        SelectDefaultPresetIfExists();
    }

    void OnPresetChanged(TraderXPreset preset)
    {
        if (preset)
        {
            preset_name = CustomizeStateService.GetInstance().GetCurrentPresetName();
            variant_name = CustomizeStateService.GetInstance().GetCurrentVariantName();
        }
        else
        {
            preset_name = "";
        }

        product_description = GetItemDescription();
        if (presetNameText) presetNameText.SetText(preset_name);
        if (variantNameText) variantNameText.SetText(variant_name);
        if (productDescriptionText) productDescriptionText.SetText(product_description);
        UpdatePlayerMoney();
    }

    void OnConfigurationChanged(TraderXProduct configuration)
    {
        currentItem = configuration;
        UpdatePlayerMoney();
    }

    override bool OnMouseButtonDown(Widget w, int x, int y, int button)
    {
        if (w != previewBtn)
            return false;

        GetGame().GetDragQueue().Call(this, "UpdateRotation");
        g_Game.GetMousePos(itemPreviewRotX, itemPreviewRotY);
        return true;
    }

    void UpdateRotation(int mouse_x, int mouse_y, bool is_dragging)
    {
        vector orientation = itemPreviewOrientation;
        orientation[0] = orientation[0] + (itemPreviewRotY - mouse_y);
        orientation[1] = orientation[1] - (itemPreviewRotX - mouse_x);

        itemPreview.SetModelOrientation(orientation);

        if (!is_dragging)
            itemPreviewOrientation = orientation;
    }

    override bool OnMouseWheel(Widget w, int x, int y, int wheel)
    {
        if (w != previewBtn)
            return false;

        itemPreviewScale = wheel;
        float ww, h, px, py;
        itemPreview.GetPos(px, py);
        itemPreview.GetSize(ww, h);
        ww = ww + (itemPreviewScale / 4);
        h = h + (itemPreviewScale / 4);
        if (ww > 0.5 && ww < 3)
        {
            itemPreview.SetSize(ww, h);
            float new_x = px - (itemPreviewScale / 8);
            float new_y = py - (itemPreviewScale / 8);
            itemPreview.SetPos(new_x, new_y);
        }
        return true;
    }
}
