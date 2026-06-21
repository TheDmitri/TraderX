class PlayerPreviewComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("tx-player-preview", PlayerPreviewComponent);

	protected PlayerPreviewWidget playerPreview;
	protected PlayerBase entPlayerPreview;
	protected vector m_CharacterOrientation;
	protected int m_CharacterRotationX;
	protected int m_CharacterRotationY;

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "tx-player-preview";
		m_Metadata.templatePath = "TraderX/datasets/gui/TraderXPrefab/PlayerPreview/PlayerPreviewView.layout";
	}

	override void OnInit()
	{
		AutoLoadWidgets();
		m_CharacterOrientation = vector.Zero;
		TraderXSelectionService.Event_OnItemSelectionChanged.Insert(OnItemSelectionChanged);
		InitPlayerPreview();
	}

	override void OnDestroy()
	{
		if (entPlayerPreview)
			GetGame().ObjectDelete(entPlayerPreview);
		TraderXSelectionService.Event_OnItemSelectionChanged.Remove(OnItemSelectionChanged);
	}

	protected void InitPlayerPreview()
	{
		CreatePreviewCharacter();
		SyncWithPlayerClothing();
	}

	protected void CreatePreviewCharacter()
	{
		if (entPlayerPreview)
			GetGame().ObjectDelete(entPlayerPreview);

		vector pos = GetGame().GetCurrentCameraPosition() - (GetGame().GetCurrentCameraDirection() * 0.5);
		pos[1] = GetGame().GetPlayer().GetPosition()[1];

		entPlayerPreview = PlayerBase.Cast(GetGame().CreateObjectEx(GetGame().GetPlayer().GetType(), pos, ECE_LOCAL|ECE_NOLIFETIME));
		if (entPlayerPreview && playerPreview)
		{
			playerPreview.SetPlayer(entPlayerPreview);
			UpdatePreviewPosition();
		}
	}

	protected void UpdatePreviewPosition()
	{
		if (!playerPreview || !entPlayerPreview) return;
		playerPreview.SetModelOrientation(m_CharacterOrientation);
		playerPreview.SetModelPosition(vector.Zero);
		playerPreview.SetSize(0.8, 0.8);
	}

	protected void SyncWithPlayerClothing()
	{
		PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
		if (!player || !entPlayerPreview) return;

		foreach (string slot : StaticTraderXCoreLists.playerAttachments)
		{
			EntityAI sourceItem = player.FindAttachmentBySlotName(slot);
			if (sourceItem)
			{
				if (slot == "Hand")
				{
					entPlayerPreview.GetHumanInventory().CreateInHands(sourceItem.GetType());
					continue;
				}
				int slotId = InventorySlots.GetSlotIdFromString(slot);
				if (slotId != -1)
					entPlayerPreview.GetInventory().CreateAttachmentEx(sourceItem.GetType(), slotId);
			}
		}
	}

	void OnItemSelectionChanged(TraderXProduct item)
	{
		if (!entPlayerPreview) return;

		EntityAI currentInHands, existingItem;
		string slotName = TraderXInventoryManager.GetSlotForPlayerPreview(item.className);
		if (slotName == string.Empty) return;

		if (item.isSelected)
		{
			existingItem = entPlayerPreview.FindAttachmentBySlotName(slotName);
			if (existingItem)
				GetGame().ObjectDelete(existingItem);

			if (slotName == "Hand")
			{
				currentInHands = entPlayerPreview.GetHumanInventory().GetEntityInHands();
				if (currentInHands)
					GetGame().ObjectDelete(currentInHands);
				entPlayerPreview.GetHumanInventory().CreateInHands(item.className);
			}
			else
			{
				entPlayerPreview.GetInventory().CreateAttachmentEx(item.className, InventorySlots.GetSlotIdFromString(slotName));
			}
		}
		else
		{
			if (slotName == "Hand")
			{
				currentInHands = entPlayerPreview.GetHumanInventory().GetEntityInHands();
				if (currentInHands && currentInHands.GetType() == item.className)
					GetGame().ObjectDelete(currentInHands);
			}
			else
			{
				existingItem = entPlayerPreview.FindAttachmentBySlotName(slotName);
				if (existingItem && existingItem.GetType() == item.className)
					GetGame().ObjectDelete(existingItem);
			}
		}

		playerPreview.UpdateItemInHands(entPlayerPreview.GetHumanInventory().GetEntityInHands());
	}

	void OnStripClick()
	{
		if (!entPlayerPreview) return;
		TraderXInventoryManager.PlayMenuSound(ETraderXSounds.SELECT);

		foreach (string slot : StaticTraderXCoreLists.playerAttachments)
		{
			EntityAI item = entPlayerPreview.FindAttachmentBySlotName(slot);
			if (item)
				GetGame().ObjectDelete(item);
		}

		EntityAI itemInHands = entPlayerPreview.GetHumanInventory().GetEntityInHands();
		if (itemInHands)
			GetGame().ObjectDelete(itemInHands);

		playerPreview.UpdateItemInHands(null);
	}

	void OnResetClick()
	{
		TraderXInventoryManager.PlayMenuSound(ETraderXSounds.SELECT);
		CreatePreviewCharacter();
		SyncWithPlayerClothing();
	}
}
