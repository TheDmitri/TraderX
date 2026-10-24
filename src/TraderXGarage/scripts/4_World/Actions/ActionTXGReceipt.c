// TraderXGarage - Vehicle receipt actions
// Pack: wrench in hands, aiming at a vehicle. Deploy: receipt in hands.
// Client conditions only decide when the action is offered; TXGReceiptService checks everything.
#ifndef TraderPlus
class ActionTXGPackVehicleCB : ActionContinuousBaseCB
{
    override void CreateActionComponent()
    {
        m_ActionData.m_ActionComponent = new CAContinuousTime(TXGConstants.RECEIPT_PACK_TIME);
    }
}

class ActionTXGPackVehicle : ActionContinuousBase
{
    void ActionTXGPackVehicle()
    {
        m_CallbackClass = ActionTXGPackVehicleCB;
        m_CommandUID = DayZPlayerConstants.CMD_ACTIONFB_CRAFTING;
        m_FullBody = true;
        m_StanceMask = DayZPlayerConstants.STANCEMASK_CROUCH;
        m_SpecialtyWeight = UASoftSkillsWeight.PRECISE_LOW;
        m_Text = "#txg_receipt_pack";
    }

    override void CreateConditionComponents()
    {
        m_ConditionTarget = new CCTNonRuined(UAMaxDistances.DEFAULT);
        m_ConditionItem = new CCINonRuined;
    }

    override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
    {
        if (GetGame().IsServer())
            return true;

        if (!GetTXGConfigService().GetClientReceiptsEnabled())
            return false;

        CarScript car = CarScript.Cast(target.GetObject());
        if (!car || !item || !item.IsKindOf("Wrench"))
            return false;

        return !GetTXGGarageService().IsCarOccupied(car);
    }

    override void OnFinishProgressServer(ActionData action_data)
    {
        CarScript car = CarScript.Cast(action_data.m_Target.GetObject());
        bool success;
        string status = GetTXGReceiptService().Pack(action_data.m_Player, car, action_data.m_MainItem, success);
        GetTXGReceiptService().TellPlayer(action_data.m_Player, status);
    }
}

class ActionTXGDeployReceiptCB : ActionContinuousBaseCB
{
    override void CreateActionComponent()
    {
        m_ActionData.m_ActionComponent = new CAContinuousTime(TXGConstants.RECEIPT_DEPLOY_TIME);
    }
}

class ActionTXGDeployReceipt : ActionContinuousBase
{
    void ActionTXGDeployReceipt()
    {
        m_CallbackClass = ActionTXGDeployReceiptCB;
        m_CommandUID = DayZPlayerConstants.CMD_ACTIONMOD_OPENITEM;
        m_CommandUIDProne = DayZPlayerConstants.CMD_ACTIONFB_OPENITEM;
        m_SpecialtyWeight = UASoftSkillsWeight.PRECISE_LOW;
        m_Text = "#txg_receipt_deploy";
    }

    override void CreateConditionComponents()
    {
        m_ConditionItem = new CCINonRuined;
        m_ConditionTarget = new CCTNone;
    }

    override bool HasTarget()
    {
        return false;
    }

    override bool HasProneException()
    {
        return true;
    }

    override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
    {
        if (GetGame().IsServer())
            return true;

        // The client may not know the vehicle yet (class sent on pickup): the server says if it is empty
        return NewReceiptBase.Cast(item) != null || ReceiptBase.Cast(item) != null;
    }

    override void OnFinishProgressServer(ActionData action_data)
    {
        bool success;
        string status = GetTXGReceiptService().Deploy(action_data.m_Player, action_data.m_MainItem, success);
        GetTXGReceiptService().TellPlayer(action_data.m_Player, status);
    }
}
#endif
