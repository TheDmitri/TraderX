// TraderXGarage - Garage Page Component
// Backs GaragePageView.layout. Lists stored vehicles, drives park-in / park-out.

class TXGGarageComponent extends DZlrComponentBase
{
    private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("txg-garage", TXGGarageComponent);

    // ForEach binding — stored vehicles.
    ref array<ref TXGVehicleRowVM> vehicles;

    // Header / status bindings.
    string vehicleCountDisplay = "0/5";
    string selectedVehicleName = "Select a vehicle";
    int    parkInCost;
    int    parkOutCost;
    string parkingStatusText = "Available";

    // Confirmation toast — text is bound; visibility is driven via widget .Show()
    // (data-visible is preview-only and emits no in-game binding). Widget members below
    // are auto-bound by data-name (AutoLoadWidgets).
    string toastText = "";
    bool   toastSuccessVisible = false;   // preview-only mirror (drives browser data-visible)
    bool   toastErrorVisible = false;
    Widget toastSuccess;
    Widget toastError;

    // Live 3D preview — bound via ItemPreviewBinding (data-bind-item). vehiclePreview holds a
    // client-side, non-networked entity; vehiclePreviewWidget is the widget itself (auto-bound,
    // used to clear the rendered model on deselect). Placeholder widgets hide while a preview shows.
    EntityAI vehiclePreview;
    ItemPreviewWidget vehiclePreviewWidget;
    Widget previewPlaceholderIcon;
    Widget previewPlaceholderText;

    // Selection state.
    protected string m_SelectedId = "";

    // The garage NPC the player interacted with (set by TXGUIService before init). Drives
    // server-side park-out spot resolution. -1 = unknown (standalone fallback to nearest).
    int m_NpcId = -1;

    override protected void RegisterMetadata()
    {
        m_Metadata = new ComponentMetadata();
        m_Metadata.selector = "txg-garage";
        m_Metadata.templatePath = "TraderXGarage/datasets/layouts/GaragePageView.layout";
    }

    // Footer "Sponsored by DayZ Beans Launcher" -> open the shared promo modal (TraderX core).
    void OnLauncher()
    {
#ifdef TraderX
        TXBeansPromoComponent.Open();
#endif
    }

    override void OnInit()
    {
        vehicles = new array<ref TXGVehicleRowVM>();

        parkInCost = GetTXGConfigService().GetClientParkInCost();
        parkOutCost = GetTXGConfigService().GetClientParkOutCost();

        TXGGarageService.Event_OnVehiclesUpdated.Insert(OnVehiclesUpdated);
        TXGGarageService.Event_OnNotify.Insert(OnNotify);
        TXGVehicleRowComponent.Event_OnVehicleSelected.Insert(OnVehicleSelected);

        OnVehiclesUpdated();
        HideToast();   // toasts start hidden until a notify arrives
        MarkForCheck();
    }

    override void OnDestroy()
    {
        TXGGarageService.Event_OnVehiclesUpdated.Remove(OnVehiclesUpdated);
        TXGGarageService.Event_OnNotify.Remove(OnNotify);
        TXGVehicleRowComponent.Event_OnVehicleSelected.Remove(OnVehicleSelected);
        GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(HideToast);
        ClearVehiclePreview();
    }

    void OnVehiclesUpdated()
    {
        array<ref TXGVehicleRowVM> cached = GetTXGGarageService().GetClientVehicles();
        vehicles = new array<ref TXGVehicleRowVM>();
        if (cached)
        {
            foreach (TXGVehicleRowVM v : cached)
            {
                if (v)
                    vehicles.Insert(v);
            }
        }

        int maxSlots = GetTXGGarageService().GetClientMaxSlots();
        vehicleCountDisplay = vehicles.Count().ToString() + "/" + maxSlots.ToString();

        // Drop selection if it no longer exists.
        if (!FindRow(m_SelectedId))
        {
            m_SelectedId = "";
            selectedVehicleName = "Select a vehicle";
            ClearVehiclePreview();
        }

        MarkForCheck();
    }

    void OnNotify(string message, bool success)
    {
        ShowToast(message, success);
    }

    // Show a sentiment-colored confirmation toast, auto-hidden after a few seconds.
    void ShowToast(string message, bool success)
    {
        toastText = message;
        toastSuccessVisible = success;
        toastErrorVisible = !success;
        if (toastSuccess) toastSuccess.Show(success);
        if (toastError) toastError.Show(!success);
        MarkForCheck();

        GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(HideToast);
        GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(HideToast, 3500, false);
    }

    void HideToast()
    {
        toastSuccessVisible = false;
        toastErrorVisible = false;
        if (toastSuccess) toastSuccess.Show(false);
        if (toastError) toastError.Show(false);
        MarkForCheck();
    }

    void OnVehicleSelected(string vehicleId)
    {
        m_SelectedId = vehicleId;
        TXGVehicleRowVM row = FindRow(vehicleId);
        if (row)
        {
            selectedVehicleName = row.displayName;
            SetVehiclePreview(row.className);
        }
        MarkForCheck();
    }

    // Spawn a client-side, non-networked preview of the vehicle and hand it to the binding.
    protected void SetVehiclePreview(string className)
    {
        ClearVehiclePreview();
        if (className == "")
            return;

        vehiclePreview = EntityAI.Cast(GetGame().CreateObjectEx(className, vector.Zero, ECE_LOCAL | ECE_NOLIFETIME));
        bool has = (vehiclePreview != null);
        if (previewPlaceholderIcon) previewPlaceholderIcon.Show(!has);
        if (previewPlaceholderText) previewPlaceholderText.Show(!has);
        MarkForCheck();
    }

    protected void ClearVehiclePreview()
    {
        if (vehiclePreview)
        {
            GetGame().ObjectDelete(vehiclePreview);
            vehiclePreview = null;
        }
        // Binding only sets on non-null — clear the rendered model directly.
        if (vehiclePreviewWidget)
            vehiclePreviewWidget.SetItem(null);
        if (previewPlaceholderIcon) previewPlaceholderIcon.Show(true);
        if (previewPlaceholderText) previewPlaceholderText.Show(true);
    }

    // Park the vehicle the player is currently near (server reads our position).
    void OnParkIn()
    {
        GetDayZularRPCManager().SendRPC(TXGConstants.RPC_NAMESPACE, "RequestParkIn", null, true, null);
    }

    // Retrieve the selected stored vehicle.
    void OnParkOut()
    {
        if (m_SelectedId == "")
        {
            ShowToast("Select a vehicle first", false);
            return;
        }

        TXGParkOutRequest req = new TXGParkOutRequest(m_SelectedId, m_NpcId);
        GetDayZularRPCManager().SendRPC(TXGConstants.RPC_NAMESPACE, "RequestParkOut", new Param1<ref TXGParkOutRequest>(req), true, null);
    }

    void OnClose()
    {
        TXGUIService svc = GetTXGUIService();
        if (svc)
            svc.CloseGarageMenu();
    }

    protected TXGVehicleRowVM FindRow(string vehicleId)
    {
        if (vehicleId == "" || !vehicles)
            return null;
        foreach (TXGVehicleRowVM v : vehicles)
        {
            if (v && v.vehicleId == vehicleId)
                return v;
        }
        return null;
    }
}
