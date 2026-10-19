// TraderXGarage - Client 3D Preview Service
// Builds client-local (ECE_LOCAL) preview vehicles with their actual stored attachments
// (wheels, doors, hood, trunk) for both the UI ItemPreviewWidget and the in-world parkSpot preview.

class TXGPreviewService extends DayZularService
{
    protected EntityAI m_UiPreviewEntity;
    protected EntityAI m_SpotPreviewEntity;
    protected bool m_SpotPreviewEnabled = true;
    protected bool m_KeepSpot = false;

    void TXGPreviewService() {}

    void ~TXGPreviewService()
    {
        ClearAll();
    }

    bool IsSpotPreviewEnabled()
    {
        return m_SpotPreviewEnabled;
    }

    void SetSpotPreviewEnabled(bool enabled)
    {
        m_SpotPreviewEnabled = enabled;
        if (!m_SpotPreviewEnabled)
            ClearSpotPreview();
    }

    EntityAI UpdatePreview(string className, array<string> attachments, int npcId)
    {
        m_KeepSpot = false;
        GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(ReleaseSpotPreview);
        ClearAll();
        if (className == "" || !TXGGarageService.IsEntityClassDefined(className))
            return null;

        vector hidePos = vector.Zero;
        Man player = GetGame().GetPlayer();
        if (player)
            hidePos = player.GetPosition() - Vector(0, 50, 0);

        m_UiPreviewEntity = CreateLocalVehicleWithParts(className, attachments, hidePos, vector.Zero);

        if (m_SpotPreviewEnabled && player)
        {
            vector spotPos;
            vector spotOri;
            if (GetTXGConfigService().ResolveClientParkSpot(npcId, player.GetPosition(), spotPos, spotOri))
            {
                m_SpotPreviewEntity = CreateLocalVehicleWithParts(className, attachments, spotPos + Vector(0, 0.15, 0), spotOri);
            }
        }

        return m_UiPreviewEntity;
    }

    protected EntityAI CreateLocalVehicleWithParts(string className, array<string> attachments, vector pos, vector ori)
    {
        int flags = ECE_LOCAL | ECE_NOLIFETIME | ECE_KEEPHEIGHT | ECE_NOSURFACEALIGN;
        Object obj = GetGame().CreateObjectEx(className, pos, flags);
        EntityAI veh = EntityAI.Cast(obj);
        if (!veh)
        {
            if (obj)
                GetGame().ObjectDelete(obj);
            return null;
        }

        veh.SetAllowDamage(false);
        if (attachments)
        {
            foreach (string partClass : attachments)
            {
                if (partClass != "" && TXGGarageService.IsEntityClassDefined(partClass))
                    AttachLocalPart(veh, partClass);
            }
        }

        dBodyDestroy(veh);
        veh.DisableSimulation(true);
        veh.SetPosition(pos);
        veh.SetOrientation(ori);
        veh.Update();
        return veh;
    }

    // Client-local entities can't use CreateAttachment (server/network path): create the part
    // locally too and move it into a free attachment slot with a LOCAL inventory take.
    protected bool AttachLocalPart(EntityAI parent, string partClass)
    {
        if (!parent || !parent.GetInventory())
            return false;

        EntityAI part = EntityAI.Cast(GetGame().CreateObjectEx(partClass, parent.GetPosition(), ECE_LOCAL | ECE_NOLIFETIME));
        if (!part)
            return false;

        InventoryLocation loc = new InventoryLocation();
        if (parent.GetInventory().FindFreeLocationFor(part, FindInventoryLocationType.ATTACHMENT, loc))
        {
            if (parent.GetInventory().TakeEntityAsAttachmentEx(InventoryMode.LOCAL, part, loc.GetSlot()))
                return true;
        }
        GetGame().ObjectDelete(part);
        return false;
    }

    // "View at spot": the menu closes but the hologram stays on the park spot for a while so the
    // player can walk around it. Cleared after KEEP_SPOT_MS or on the next preview.
    static const int KEEP_SPOT_MS = 30000;

    void KeepSpotPreview()
    {
        if (!m_SpotPreviewEntity)
            return;
        m_KeepSpot = true;
        GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(ReleaseSpotPreview);
        GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(ReleaseSpotPreview, KEEP_SPOT_MS, false);
    }

    EntityAI GetSpotPreview()
    {
        return m_SpotPreviewEntity;
    }

    bool HasSpotPreview()
    {
        return m_SpotPreviewEntity != null;
    }

    protected void ReleaseSpotPreview()
    {
        m_KeepSpot = false;
        ClearSpotPreview();
    }

    void ClearSpotPreview()
    {
        if (m_SpotPreviewEntity)
        {
            GetGame().ObjectDelete(m_SpotPreviewEntity);
            m_SpotPreviewEntity = null;
        }
    }

    void ClearAll()
    {
        if (m_UiPreviewEntity)
        {
            GetGame().ObjectDelete(m_UiPreviewEntity);
            m_UiPreviewEntity = null;
        }
        if (!m_KeepSpot)
            ClearSpotPreview();
    }
}

static ref TXGPreviewService g_TXGPreviewService;

TXGPreviewService GetTXGPreviewService()
{
    if (!g_TXGPreviewService)
        g_TXGPreviewService = TXGPreviewService.Cast(DZlrServiceRegistry.GetInstance().GetService(TXGPreviewService));
    return g_TXGPreviewService;
}
