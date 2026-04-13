class InsurancePageComponent extends DZlrComponentBase
{
    private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("traderx-insurance-page", InsurancePageComponent);

    ref array<ref TraderXInsurancePolicy> m_Policies;

    // Widgets
    Widget policyListContainer;
    TextWidget noPoliciesText, balanceText;
    ButtonWidget refreshBtn;

    override protected void RegisterMetadata()
    {
        m_Metadata = new ComponentMetadata();
        m_Metadata.selector = "traderx-insurance-page";
        m_Metadata.templatePath = "TraderX/datasets/gui/Insurance/InsurancePageView.layout";
    }

    override void OnInit()
    {
        m_Policies = new array<ref TraderXInsurancePolicy>();

        // Request policies from server
        GetDayZularRPCManager().SendRPC("TraderX", "GetPlayerPoliciesRequest", new Param1<int>(TraderXTradingService.GetInstance().GetNpcId()), true, null);

        // Subscribe to response
        // The response will be handled by a client-side RPC handler that updates our list
    }

    void SetPolicies(array<ref TraderXInsurancePolicy> policies)
    {
        m_Policies = policies;
        RefreshPolicyList();
    }

    void RefreshPolicyList()
    {
        // Clear existing list
        if (policyListContainer)
        {
            Widget child = policyListContainer.GetChildren();
            while (child)
            {
                Widget next = child.GetSibling();
                child.Unlink();
                child = next;
            }
        }

        if (!m_Policies || m_Policies.Count() == 0)
        {
            if (noPoliciesText) noPoliciesText.Show(true);
            return;
        }

        if (noPoliciesText) noPoliciesText.Show(false);

        // TODO: Create policy card components for each policy
        // For each policy, display: vehicleType, status, premium paid, payout amount
        // Active policies with missing vehicles get a "Claim" button
    }

    void OnClaimPolicy(string policyId)
    {
        GetDayZularRPCManager().SendRPC("TraderX", "ClaimInsuranceRequest", new Param1<string>(policyId), true, null);
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (w == refreshBtn)
        {
            GetDayZularRPCManager().SendRPC("TraderX", "GetPlayerPoliciesRequest", new Param1<int>(TraderXTradingService.GetInstance().GetNpcId()), true, null);
            return true;
        }
        return false;
    }
}
