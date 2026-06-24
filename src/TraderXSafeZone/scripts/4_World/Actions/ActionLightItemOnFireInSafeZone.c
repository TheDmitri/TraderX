class ActionLightItemOnFireInTXSafeZoneCB : ActionContinuousBaseCB
{
    override void CreateActionComponent()
    {
        m_ActionData.m_ActionComponent = new CAContinuousTime(UATimeSpent.FIREPLACE_IGNITE);
    }
}

class ActionLightItemOnFireInTXSafeZone: ActionContinuousBase
{
    void ActionLightItemOnFireInTXSafeZone()
    {
        m_CallbackClass = ActionLightItemOnFireInTXSafeZoneCB;
        m_CommandUID = DayZPlayerConstants.CMD_ACTIONFB_STARTFIRE;
        m_FullBody = true;
        m_StanceMask = DayZPlayerConstants.STANCEMASK_CROUCH;
        m_SpecialtyWeight = UASoftSkillsWeight.ROUGH_HIGH;
    }

    override void CreateConditionComponents()
    {
        m_ConditionTarget = new CCTNonRuined(UAMaxDistances.DEFAULT);
        m_ConditionItem = new CCINonRuined;
    }

    override string GetText()
    {
        return "#ignite";
    }

    override bool CanBePerformedFromQuickbar()
    {
        return true;
    }

    override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
    {
        if(GetGame().IsServer())
            return true;

        FireplaceBase target_item = FireplaceBase.Cast(target.GetObject());
        if (target_item && item)
        {
            if (!target_item.IsIgnited() && player.IsInTXSafeZone())
                return true;
        }
        return false;
    }

    override void OnFinishProgressServer(ActionData action_data)
    {
        FireplaceBase target_item = FireplaceBase.Cast(action_data.m_Target.GetObject());
        ItemBase item = action_data.m_MainItem;

        if(target_item)
        {
            ItemBase firewood = ItemBase.Cast(target_item.GetInventory().CreateAttachment("Firewood"));
            if(firewood)
                firewood.SetQuantityMax();
        }
        
        if (target_item.IsWet())
            target_item.SetWet(target_item.GetWetMin());

        target_item.GetInventory().CreateAttachment("WoodenStick");
        target_item.GetInventory().CreateAttachment("Rag");
        target_item.StartFire(true);
    }

    override bool SetupAction(PlayerBase player, ActionTarget target, ItemBase item, out ActionData action_data, Param extra_data = NULL)
    {
        if(super.SetupAction(player, target, item, action_data, extra_data))
        {
            ItemBase target_item = ItemBase.Cast(target.GetObject());
            if (target_item)
            {
                SetIgnitingAnimation(target_item);
            }

            return true;
        }

        return false;
    }

    void SetIgnitingAnimation(ItemBase target_item)
    {
        if(target_item.HasFlammableMaterial())
        {
            m_CommandUID = DayZPlayerConstants.CMD_ACTIONFB_STARTFIRE;
        }
        else
        {
            m_CommandUID = DayZPlayerConstants.CMD_ACTIONFB_CRAFTING;
        }
    }
}
