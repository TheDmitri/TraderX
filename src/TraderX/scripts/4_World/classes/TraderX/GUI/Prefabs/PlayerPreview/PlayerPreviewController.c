class PlayerPreviewController extends DZlrComponentBase
{
    private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("traderx-player-preview", PlayerPreviewController);

    protected PlayerPreviewWidget playerPreview;
    protected PlayerBase entPlayerPreview;
    protected vector m_CharacterOrientation;
    protected int m_CharacterRotationX;
    protected int m_CharacterRotationY;

    static ref PlayerPreviewController m_Instance;

    ButtonWidget stripBtn, resetBtn;

    static PlayerPreviewController GetInstance()
    {
        if (!m_Instance)
            m_Instance = new PlayerPreviewController();
        return m_Instance;
    }

    override protected void RegisterMetadata()
    {
        m_Metadata = new ComponentMetadata();
        m_Metadata.selector = "traderx-player-preview";
        m_Metadata.templatePath = "";
    }

    void PlayerPreviewController()
    {
        m_CharacterOrientation = vector.Zero;
        m_Instance = this;
        TraderXSelectionService.Event_OnItemSelectionChanged.Insert(OnItemSelectionChanged);
    }

    void ~PlayerPreviewController()
    {
        if (entPlayerPreview)
            GetGame().ObjectDelete(entPlayerPreview);

        TraderXSelectionService.Event_OnItemSelectionChanged.Remove(OnItemSelectionChanged);
        if (m_Instance == this)
            m_Instance = null;
    }

    override void OnInit()
    {
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

        vector pos = GetGame().GetCurrentCameraPosition() - (GetGame().GetCurrentCameraDirection() * 0.5);
        pos[1] = GetGame().GetPlayer().GetPosition()[1];

        entPlayerPreview = PlayerBase.Cast(GetGame().CreateObjectEx(GetGame().GetPlayer().GetType(), pos, ECE_LOCAL | ECE_NOLIFETIME));
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
        if (slotName != string.Empty)
        {
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
    }

    bool OnMouseButtonDown(Widget w, int x, int y, int button)
    {
        if (w == playerPreview)
        {
            GetGame().GetDragQueue().Call(this, "UpdatePlayerRotation");
            g_Game.GetMousePos(m_CharacterRotationX, m_CharacterRotationY);
            return true;
        }
        return false;
    }

    void UpdatePlayerRotation(int mouse_x, int mouse_y, bool is_dragging)
    {
        vector orientation = m_CharacterOrientation;
        orientation[1] = orientation[1] - (m_CharacterRotationX - mouse_x);

        playerPreview.SetModelOrientation(orientation);

        if (!is_dragging)
            m_CharacterOrientation = orientation;
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (w == stripBtn)
        {
            if (!entPlayerPreview) return false;
            TraderXInventoryManager.PlayMenuSound(ETraderXSounds.SELECT);

            foreach (string slot : StaticTraderXCoreLists.playerAttachments)
            {
                EntityAI item = entPlayerPreview.FindAttachmentBySlotName(slot);
                if (item) GetGame().ObjectDelete(item);
            }

            EntityAI itemInHands = entPlayerPreview.GetHumanInventory().GetEntityInHands();
            if (itemInHands) GetGame().ObjectDelete(itemInHands);
            playerPreview.UpdateItemInHands(null);
            return true;
        }

        if (w == resetBtn)
        {
            TraderXInventoryManager.PlayMenuSound(ETraderXSounds.SELECT);
            CreatePreviewCharacter();
            SyncWithPlayerClothing();
            return true;
        }

        return false;
    }
}
