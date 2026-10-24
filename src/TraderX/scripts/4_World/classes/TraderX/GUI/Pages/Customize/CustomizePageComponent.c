// Customize page shell (DayZular). Hosts three swappable tabs
// (presets / variants / attachments) via ComponentWidgetBinding driven by
// {{ current_tab }}, plus a full-page modal surface driven by {{ current_modal }}.
// Tab navigation uses NavButtonBinding -> SelectComponent. The cart/total/purchase
// is the shared tx-checkout component embedded in the layout; this shell only owns
// the item preview, the descriptive bindings and the tab/modal swap state.
class CustomizePageComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("tx-customize-page", CustomizePageComponent);

	static ref CustomizePageComponent instance;

	ref TraderXProduct originalItem;
	ref TraderXProduct currentItem;
	EntityAI preview;

	// Active tab / modal selectors — drive the two ComponentWidgetBinding hosts in the layout.
	string current_tab = "tx-customize-presets";
	string current_modal = "";

	// Tab the page should open on, set by the card affordance icons before mount:
	// palette -> variants, wrench -> attachments, gear/right-click -> presets. Consumed
	// once in OnInit. Empty = default (presets).
	static string s_PendingTab = "";

	// Display bindings
	string product_classname;
	string base_price;
	string product_description;
	string preset_name;
	string variant_name;
	// Nasdara: title, breadcrumb, tab titles and subtitles ("2 saved", "4 of 5 slots")
	string product_title;
	string breadcrumb_path;
	string tab_presets_title;
	string tab_variants_title;
	string tab_attachments_title;
	string tab_presets_sub;
	string tab_attachments_sub;
	Widget presetsTabBg;
	Widget variantsTabBg;
	Widget attachmentsTabBg;
	Widget presetsTabLine;
	Widget variantsTabLine;
	Widget attachmentsTabLine;
	TextWidget presetsTabTitle;
	TextWidget variantsTabTitle;
	TextWidget attachmentsTabTitle;

	// Preview widgets (AutoLoadWidgets)
	ItemPreviewWidget itemPreview;
	ButtonWidget previewBtn;

	// Tab buttons (AutoLoadWidgets) — recolored on tab switch for active-state feedback.
	ButtonWidget tabPresetsBtn;
	ButtonWidget tabVariantsBtn;
	ButtonWidget tabAttachmentsBtn;

	private vector itemPreviewOrientation;
	private int itemPreviewRotX;
	private int itemPreviewRotY;
	private int itemPreviewScale;

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "tx-customize-page";
		m_Metadata.templatePath = "TraderX/datasets/gui/PurchasePage/CustomizePageVariant.layout";
	}

	static CustomizePageComponent GetInstance()
	{
		return instance;
	}

	override void OnInit()
	{
		instance = this;
		AutoLoadWidgets();

		// The selected product is published to CustomizeStateService by the purchase
		// page before this component is mounted (see PurchasePageComponent.OnItemCardRClick).
		originalItem = CustomizeStateService.GetInstance().GetOriginalItem();
		if (!originalItem)
		{
			GetTraderXLogger().LogError("CustomizePageComponent.OnInit - No original item set in CustomizeStateService");
			return;
		}
		currentItem = originalItem;

		RegisterServiceEvents();

		TraderXUINavigationService.GetInstance().SetNavigationId(ENavigationIds.CUSTOMIZE);
		TraderXTradingService.GetInstance().SetTradeMode(ETraderXTradeMode.BUY);
		TraderXTradingService.GetInstance().SetMaxQuantity(false);

		// Snapshot the purchase cart, then clear it so customize builds its own
		// isolated configuration selection. OnClose restores the snapshot.
		TraderXSelectionService.GetInstance().PushSelectionSnapshot();
		TraderXSelectionService.GetInstance().DeSelectAllItems();

		originalItem.defaultPreset = null;
		TraderXSelectionService.GetInstance().SelectItem(originalItem);

		product_classname = originalItem.GetDisplayName();
		product_description = CustomizePreviewService.GetInstance().GetItemDescription();
		base_price = TraderXCurrencyService.GetInstance().GetTraderCurrencySymbol() + TraderXQuantityManager.GetFormattedMoneyAmount(originalItem.buyPrice);
		product_title = product_classname;
		product_title.ToUpper();
		breadcrumb_path = Widget.TranslateString("#tpm_purchase") + " / ";
		TraderXCategory category = FindCategoryOf(originalItem);
		if (category) breadcrumb_path += Widget.TranslateString(category.categoryName) + " / ";
		tab_presets_title = CheckoutComponent.Upper("#tpm_presets");
		tab_variants_title = CheckoutComponent.Upper("#tpm_variants");
		tab_attachments_title = CheckoutComponent.Upper("#tpm_attachments");
		CustomizePreviewService.Event_OnAttachmentsChanged.Insert(OnAttachmentsChanged);
		UpdateTabSubtitles();

		// The preview entity is created by CustomizeStateService.Initialize before this
		// component mounts, so the OnPreviewUpdated event already fired — seed it directly.
		preview = CustomizePreviewService.GetInstance().GetCurrentPreview();

		// Land on the tab the card icon requested (palette->variants, wrench->attachments),
		// else the default presets tab. Consumed once.
		if (s_PendingTab != "")
		{
			current_tab = s_PendingTab;
			s_PendingTab = "";
		}

		// the tx-checkout child mounted before the nav id was CUSTOMIZE
		if (CheckoutComponent.GetInstance()) CheckoutComponent.GetInstance().UpdateTradeModeText();
		SelectDefaultPresetIfExists();
		UpdateTabHighlight();
		MarkForCheck();
	}

	override void OnDestroy()
	{
		UnregisterServiceEvents();
		CustomizePreviewService.Event_OnAttachmentsChanged.Remove(OnAttachmentsChanged);
		// The preview entity is OWNED by CustomizePreviewService (it created it and
		// still holds m_CurrentPreview). Deleting it here left the service with a
		// dangling pointer, and the next CreatePreview -> CleanupCurrentPreview deleted
		// it a second time. Let the owner clean up; we only drop our reference.
		CustomizePreviewService.GetInstance().CleanupCurrentPreview();
		preview = null;
		// Cleanup must run on EVERY exit, not just the close button. Leaving via the
		// top nav (Purchase/Sell) unmounts this component without calling OnClose, which
		// otherwise leaked the snapshot and left stale customize identity behind.
		// Both calls are idempotent: RestoreSelectionSnapshot no-ops once the snapshot is
		// consumed (OnClose), and Clear() is safe to repeat.
		TraderXSelectionService.GetInstance().RestoreSelectionSnapshot();
		CustomizeStateService.GetInstance().Clear();
		if (instance == this)
			instance = null;
	}

	void RegisterServiceEvents()
	{
		UnregisterServiceEvents();
		CustomizePreviewService.Event_OnPreviewUpdated.Insert(OnPreviewUpdated);
		CustomizeStateService.Event_OnVariantChanged.Insert(OnVariantChanged);
		CustomizeStateService.Event_OnPresetChanged.Insert(OnPresetChanged);
		CustomizeStateService.Event_OnConfigurationChanged.Insert(OnConfigurationChanged);
	}

	void UnregisterServiceEvents()
	{
		CustomizePreviewService.Event_OnPreviewUpdated.Remove(OnPreviewUpdated);
		CustomizeStateService.Event_OnVariantChanged.Remove(OnVariantChanged);
		CustomizeStateService.Event_OnPresetChanged.Remove(OnPresetChanged);
		CustomizeStateService.Event_OnConfigurationChanged.Remove(OnConfigurationChanged);
	}

	TraderXProduct GetItem()
	{
		return currentItem;
	}

	// === Tab navigation (NavButtonBinding calls SelectComponent on this host) ===

	void SelectComponent(string selector)
	{
		if (current_tab == selector)
			return;
		current_tab = selector;
		Event_OnComponentChanged.Invoke(selector);
		UpdateTabHighlight();
		UpdateTabSubtitles();
		MarkForCheck();
	}

	// Tab buttons use the standard ButtonBinding (data-on-click) path. NavButtonBinding
	// was not wired in-game, so the tabs are driven through these handlers instead.
	void OnTabPresets()
	{
		SelectComponent("tx-customize-presets");
	}

	void OnTabVariants()
	{
		SelectComponent("tx-customize-variants");
	}

	void OnTabAttachments()
	{
		SelectComponent("tx-customize-attachments");
	}

	// Active tab: Raised fill, cream title, ember underline.
	void UpdateTabHighlight()
	{
		ApplyTab(current_tab == "tx-customize-presets", presetsTabBg, presetsTabLine, presetsTabTitle);
		ApplyTab(current_tab == "tx-customize-variants", variantsTabBg, variantsTabLine, variantsTabTitle);
		ApplyTab(current_tab == "tx-customize-attachments", attachmentsTabBg, attachmentsTabLine, attachmentsTabTitle);
	}

	void ApplyTab(bool active, Widget bg, Widget line, TextWidget title)
	{
		if (bg) bg.Show(active);
		if (line) line.Show(active);
		if (title) title.SetColor(DZTernaryInt.If(active, TraderXTheme.NAV_ACTIVE, TraderXTheme.NAV_INACTIVE));
	}

	// "2 saved" / "4 of 5 slots"
	void UpdateTabSubtitles()
	{
		if (!originalItem) return;
		int presets = TraderXPresetsService.GetInstance().GetPresets(originalItem.productId).Count();
		TraderXPresets serverPresets = TraderXPresetsService.GetInstance().GetServerPresets(originalItem.productId);
		if (serverPresets && serverPresets.presets) presets += serverPresets.presets.Count();
		tab_presets_sub = string.Format(Widget.TranslateString("#tpm_presets_saved"), presets);

		array<ref TXCustomizeSlot> slots = new array<ref TXCustomizeSlot>();
		TXCustomizeSlot.Build(originalItem, slots);
		int fitted = 0;
		foreach (TXCustomizeSlot slot : slots)
		{
			if (slot.GetFitted()) fitted++;
		}
		tab_attachments_sub = string.Format(Widget.TranslateString("#tpm_slots_fitted"), fitted, slots.Count());
		MarkForCheck();
	}

	void OnAttachmentsChanged(array<ref TraderXProduct> attachments)
	{
		UpdateTabSubtitles();
	}

	static TraderXCategory FindCategoryOf(TraderXProduct item)
	{
		if (!item) return null;
		foreach (TraderXCategory category : TraderXTradingService.GetInstance().GetTraderCategories())
		{
			if (category && category.productIds && category.productIds.Find(item.productId) != -1)
				return category;
		}
		return null;
	}

	// Close the customize overlay and return to the underlying page.
	void OnClose()
	{
		TraderXInventoryManager.PlayMenuSound(ETraderXSounds.SELECT);
		// Discard the customize configuration and restore the purchase cart.
		TraderXSelectionService.GetInstance().RestoreSelectionSnapshot();
		TraderXTradingService.GetInstance().SetMaxQuantity(true);
		TraderXAppComponent.GetInstance().CloseSubPage();
	}

	// === Modal surface (driven by {{ current_modal }}) ===

	void ShowModal(string selector)
	{
		current_modal = selector;
		MarkForCheck();
	}

	void HideModal()
	{
		current_modal = "";
		MarkForCheck();
	}

	void ShowConfirmModal(string title, string content)
	{
		ModalConfirmationComponent.s_PendingTitle = title;
		ModalConfirmationComponent.s_PendingContent = content;
		current_modal = "tx-modal-confirm";
		MarkForCheck();
	}

	// === Default preset auto-selection (shared, needs originalItem) ===

	void SelectDefaultPresetIfExists()
	{
		string storageProductId = originalItem.productId;
		TraderXPreset defaultPreset = TraderXPresetsService.GetInstance().GetDefaultPreset(storageProductId);
		if (!defaultPreset)
			defaultPreset = TraderXPresetsService.GetInstance().GetServerDefaultPreset(storageProductId);
		if (!defaultPreset) return;

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

	// === Service event handlers (update the shell's display bindings) ===

	void OnPreviewUpdated(EntityAI newPreview)
	{
		preview = newPreview;
		MarkForCheck();
	}

	void OnVariantChanged(TraderXProduct variant)
	{
		currentItem = variant;
		variant_name = CustomizeStateService.GetInstance().GetCurrentVariantName();
		product_description = CustomizePreviewService.GetInstance().GetItemDescription();
		MarkForCheck();
		SelectDefaultPresetIfExists();
	}

	void OnPresetChanged(TraderXPreset preset)
	{
		UpdateTabSubtitles();
		if (preset)
		{
			preset_name = CustomizeStateService.GetInstance().GetCurrentPresetName();
			variant_name = CustomizeStateService.GetInstance().GetCurrentVariantName();
		}
		else
		{
			preset_name = "";
		}
		product_description = CustomizePreviewService.GetInstance().GetItemDescription();
		MarkForCheck();
	}

	void OnConfigurationChanged(TraderXProduct configuration)
	{
		currentItem = configuration;
		UpdateTabSubtitles();
		MarkForCheck();
	}

	// "Save as preset" in the configuration panel: the presets tab owns the name dialog.
	void OpenSavePreset()
	{
		if (current_tab == "tx-customize-presets")
		{
			foreach (DZlrComponentBase child : m_ChildComponents)
			{
				TXCustomizePresetsTabComponent tab = TXCustomizePresetsTabComponent.Cast(child);
				if (tab)
				{
					tab.OnNewPresetClick();
					return;
				}
			}
		}
		TXCustomizePresetsTabComponent.s_OpenCreateOnInit = true;
		OnTabPresets();
	}

	// Item preview rotation/scale is handled by the ItemPreviewBinding
	// (Enable_Rotation/Enable_Scale) on the itemPreview widget in the layout.
}
