class PlayerPreviewController: ViewController
{
    protected PlayerPreviewWidget playerPreview;
    protected PlayerBase entPlayerPreview;
    protected vector m_CharacterOrientation;
    protected int m_CharacterRotationX;
    protected int m_CharacterRotationY;

    static ref PlayerPreviewController m_Instance;

    static PlayerPreviewController GetInstance()
    {
        if (!m_Instance)
            m_Instance = new PlayerPreviewController();
        return m_Instance;
    }
    
    void PlayerPreviewController()
    {
        m_CharacterOrientation = vector.Zero;
        TraderXSelectionService.Event_OnItemSelectionChanged.Insert(OnItemSelectionChanged);
    }

    void ~PlayerPreviewController()
    {
        if (entPlayerPreview)
            GetGame().ObjectDelete(entPlayerPreview);
            
        TraderXSelectionService.GetInstance().Event_OnItemSelectionChanged.Remove(OnItemSelectionChanged);
    }

    override void OnWidgetScriptInit(Widget w)
    {
        super.OnWidgetScriptInit(w);
        InitPlayerPreview();
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

        //Create preview character at a safe position
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

        //Sync all attachments from player to preview
        array<string> attachmentSlots = new array<string>();
        foreach (string slot: StaticTraderXCoreLists.playerAttachments)
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

    protected void ApplySelectedItems()
    {
        array<ref TraderXProduct> selectedItems = TraderXSelectionService.GetInstance().GetSelectedItems();
        foreach (TraderXProduct product: selectedItems)
        {
            string slotName = TraderXInventoryManager.GetSlotForPlayerPreview(product.className);
            if (slotName != string.Empty)
            {
                //Remove existing item in slot if any
                EntityAI existingItem = entPlayerPreview.FindAttachmentBySlotName(slotName);
                if (existingItem)
                    GetGame().ObjectDelete(existingItem);

                //Create new item
                if (slotName == "Hand")
                {
                    EntityAI currentInHands = entPlayerPreview.GetHumanInventory().GetEntityInHands();
                    if (currentInHands)
                        GetGame().ObjectDelete(currentInHands);
                    entPlayerPreview.GetHumanInventory().CreateInHands(product.className);
                }
                else
                {
                    entPlayerPreview.GetInventory().CreateAttachmentEx(product.className, InventorySlots.GetSlotIdFromString(slotName));
                }
            }
        }
        
        playerPreview.UpdateItemInHands(entPlayerPreview.GetHumanInventory().GetEntityInHands());
    }

    void OnItemSelectionChanged(TraderXProduct item)
    {
        if (!entPlayerPreview) return;

        EntityAI currentInHands, existingItem;
        
        string slotName = TraderXInventoryManager.GetSlotForPlayerPreview(item.className);
        if (slotName != string.Empty)
        {
            if (item.isSelected)
            {
                //Remove existing item in slot
                existingItem = entPlayerPreview.FindAttachmentBySlotName(slotName);
                if (existingItem)
                    GetGame().ObjectDelete(existingItem);

                //Create new item
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
                //Item was deselected, remove it
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
    }

    bool OnMouseButtonDown(Widget w, int x, int y, int button)
    {
      if(w == playerPreview)
      {
        GetGame().GetDragQueue().Call(this, "UpdatePlayerRotation");
        g_Game.GetMousePos( m_CharacterRotationX, m_CharacterRotationY );
        return true;
      }
      return false;
    }

    void UpdatePlayerRotation(int mouse_x, int mouse_y, bool is_dragging)
    {
        vector orientation = m_CharacterOrientation;
		orientation[1] = orientation[1] - ( m_CharacterRotationX - mouse_x );

		playerPreview.SetModelOrientation( orientation );

		if ( !is_dragging )
			m_CharacterOrientation = orientation;
    }

    bool OnStripExecute(ButtonCommandArgs args)
    {
        if (!entPlayerPreview) return false;

        TraderXInventoryManager.PlayMenuSound(ETraderXSounds.SELECT);
        
        //Remove all clothing items
        array<string> attachmentSlots = new array<string>();        
        foreach (string slot: StaticTraderXCoreLists.playerAttachments)
        {
            EntityAI item = entPlayerPreview.FindAttachmentBySlotName(slot);
            if (item)
                GetGame().ObjectDelete(item);
        }
        
        //Clear hands
        EntityAI itemInHands = entPlayerPreview.GetHumanInventory().GetEntityInHands();
        if (itemInHands)
            GetGame().ObjectDelete(itemInHands);
            
        playerPreview.UpdateItemInHands(null);
        return true;
    }

    bool OnResetExecute(ButtonCommandArgs args)
    {
        TraderXInventoryManager.PlayMenuSound(ETraderXSounds.SELECT);
        CreatePreviewCharacter();
        SyncWithPlayerClothing();
        return true;
    }
}