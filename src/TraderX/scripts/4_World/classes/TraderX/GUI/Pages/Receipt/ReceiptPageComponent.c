class ReceiptPageComponent extends DZlrComponentBase
{
    private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("traderx-receipt-page", ReceiptPageComponent);

    ref array<ref TraderXReceipt> m_Receipts;

    // Widgets
    Widget receiptListContainer;
    TextWidget noReceiptsText;
    ButtonWidget refreshBtn;

    override protected void RegisterMetadata()
    {
        m_Metadata = new ComponentMetadata();
        m_Metadata.selector = "traderx-receipt-page";
        m_Metadata.templatePath = "TraderX/datasets/gui/Receipt/ReceiptPageView.layout";
    }

    override void OnInit()
    {
        m_Receipts = new array<ref TraderXReceipt>();

        // Subscribe to receipt list response
        TraderXReceiptService.Event_OnReceiptsReceived.Insert(OnReceiptsReceived);
        TraderXReceiptService.Event_OnRedeemResult.Insert(OnRedeemResult);

        // Request receipts from server
        GetDayZularRPCManager().SendRPC("TraderX", "GetPlayerReceiptsRequest", new Param1<int>(TraderXTradingService.GetInstance().GetNpcId()), true, null);
    }

    override void OnDestroy()
    {
        TraderXReceiptService.Event_OnReceiptsReceived.Remove(OnReceiptsReceived);
        TraderXReceiptService.Event_OnRedeemResult.Remove(OnRedeemResult);
    }

    void OnReceiptsReceived(array<ref TraderXReceipt> receipts)
    {
        m_Receipts = receipts;
        RefreshReceiptList();
    }

    void OnRedeemResult(bool success, string message)
    {
        if (success)
        {
            // Refresh the list after successful redeem
            GetDayZularRPCManager().SendRPC("TraderX", "GetPlayerReceiptsRequest", new Param1<int>(TraderXTradingService.GetInstance().GetNpcId()), true, null);
        }

        // Show notification
        TraderXNotificationService.GetInstance().ShowNotification(
            TernaryString(success, "Receipt Redeemed", "Redeem Failed"),
            message,
            TernaryInt(success, ENotificationType.SUCCESS, ENotificationType.ERROR)
        );
    }

    void RefreshReceiptList()
    {
        if (receiptListContainer)
        {
            Widget child = receiptListContainer.GetChildren();
            while (child)
            {
                Widget next = child.GetSibling();
                child.Unlink();
                child = next;
            }
        }

        if (!m_Receipts || m_Receipts.Count() == 0)
        {
            if (noReceiptsText) noReceiptsText.Show(true);
            return;
        }

        if (noReceiptsText) noReceiptsText.Show(false);

        // TODO: Create receipt card components for each pending receipt
        // Display: vehicleType, buyPrice, expiry date, Redeem button
    }

    void OnRedeemReceipt(string receiptId)
    {
        int npcId = TraderXTradingService.GetInstance().GetNpcId();
        GetDayZularRPCManager().SendRPC("TraderX", "RedeemReceiptRequest", new Param2<string, int>(receiptId, npcId), true, null);
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (w == refreshBtn)
        {
            GetDayZularRPCManager().SendRPC("TraderX", "GetPlayerReceiptsRequest", new Param1<int>(TraderXTradingService.GetInstance().GetNpcId()), true, null);
            return true;
        }
        return false;
    }
}
