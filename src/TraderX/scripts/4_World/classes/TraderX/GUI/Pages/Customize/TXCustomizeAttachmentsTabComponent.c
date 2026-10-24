// Customize attachments tab. Renders the product's compatible attachments directly
// as item cards in two sibling grids (1-col list / 5-col tiles) toggled by the
// global view mode, with an embedded tx-view-toggle. Clicking an attachment item
// card toggles it on the preview through CustomizePreviewService.
class TXCustomizeAttachmentsTabComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("tx-customize-attachments", TXCustomizeAttachmentsTabComponent);

	ref array<ref TraderXProduct> _inputAttachments = new array<ref TraderXProduct>();
	// Nasdara: slot rows and the preset chips below them.
	ref array<ref TXCustomizeSlot> _inputSlots = new array<ref TXCustomizeSlot>();
	ref array<ref TraderXPreset> _inputPresetChips = new array<ref TraderXPreset>();
	string presets_label;
	string empty_label;
	ref TraderXProduct originalItem;

	// List/grid view containers (toggled by TraderXViewModeService).
	Widget attachmentGridList;
	Widget attachmentGridTiles;

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "tx-customize-attachments";
		m_Metadata.templatePath = "TraderX/datasets/gui/PurchasePage/Customize/CustomizeAttachmentsTabView.layout";
	}

	override void OnInit()
	{
		AutoLoadWidgets();
		originalItem = CustomizeStateService.GetInstance().GetOriginalItem();
		ItemCardComponent.Event_OnItemCardClickEventCallBack.Insert(OnItemCardSelected);
		TraderXViewModeService.Event_OnViewModeChangedCallBack.Insert(OnViewModeChanged);
		FillAttachmentList();
	}

	override void OnDestroy()
	{
		ItemCardComponent.Event_OnItemCardClickEventCallBack.Remove(OnItemCardSelected);
		TraderXViewModeService.Event_OnViewModeChangedCallBack.Remove(OnViewModeChanged);
	}

	void OnViewModeChanged(bool grid)
	{
		TraderXViewModeBinder.Apply(attachmentGridList, attachmentGridTiles);
	}

	void FillAttachmentList()
	{
		if (!originalItem) return;

		_inputAttachments.Clear();
		array<ref TraderXProduct> attachments = originalItem.GetAttachments();
		foreach (TraderXProduct attachment : attachments)
			_inputAttachments.Insert(attachment);
		TXCustomizeSlot.Build(originalItem, _inputSlots);

		_inputPresetChips.Clear();
		TraderXPresets serverPresets = TraderXPresetsService.GetInstance().GetServerPresets(originalItem.productId);
		if (serverPresets && serverPresets.presets)
		{
			foreach (TraderXPreset serverPreset : serverPresets.presets)
				_inputPresetChips.Insert(serverPreset);
		}
		foreach (TraderXPreset userPreset : TraderXPresetsService.GetInstance().GetPresets(originalItem.productId))
			_inputPresetChips.Insert(userPreset);
		presets_label = CheckoutComponent.Upper("#tpm_your_presets");

		empty_label = TernaryString(_inputAttachments.Count() == 0, "#tpm_no_attachments", "");
		MarkForCheck();
	}

	bool IsAttachment(TraderXProduct product)
	{
		if (!product) return false;
		foreach (TraderXProduct att : _inputAttachments)
		{
			if (att.productId == product.productId) return true;
		}
		return false;
	}

	void OnItemCardSelected(ItemCardComponent itemSelected, int command)
	{
		if (command != ETraderXClickEvents.LCLICK)
			return;

		TraderXProduct attachment = itemSelected.GetItem();
		if (!IsAttachment(attachment))
			return;

		if (CustomizePreviewService.GetInstance().HasAttachment(attachment))
			CustomizePreviewService.GetInstance().RemoveAttachment(attachment);
		else
			CustomizePreviewService.GetInstance().AddAttachment(attachment);
	}
}
