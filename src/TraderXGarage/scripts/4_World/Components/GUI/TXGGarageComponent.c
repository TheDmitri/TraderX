// TraderXGarage - Garage Page Component
// Backs GaragePageView.layout (UX pass 2).
// Left : the vehicle at the garage door (park it from its card) + stored vehicles.
// Right: the selected vehicle — preview, condition tiles, ONE gold action, quiet service buttons.
// Insurance / share panels replace the preview; the camera view hides the window and keeps a bar.

class TXGGarageComponent extends DZlrComponentBase
{
    private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("txg-garage", TXGGarageComponent);

    // ForEach bindings.
    ref array<ref TXGVehicleRowVM> vehicles;
    ref array<ref TXGSharePlayerVM> sharePlayers;

    // Text bindings.
    string balanceText = "";
    string vehicleCountDisplay = "0 / 5";
    string nearbyNameText = "";
    string nearbyStateText = "";
    string nearbyStatusText = "";
    string nearbyParkText = "#txg_park_in";
    string selectedVehicleName = "";
    string selectedBadgesText = "";
    string tileBodyText = "-";
    string tileEngineText = "-";
    string tileFuelText = "-";
    string tilePartsText = "-";
    string primaryText = "";
    string primaryPriceText = "";
    string repairBtnText = "#txg_service";
    string insureBtnText = "#txg_insurance";
    string shareBtnText = "#txg_share_access";
    string servicesHintText = "";
    string insStatusText = "";
    string insCoveredText = "";
    string insPerDayText = "";
    string insDeductibleText = "";
    string insDaysText = "";
    string insUntilText = "";
    string insConfirmText = "";
    string insTotalText = "";
    string toastText = "";
    // Browser-preview mirrors (in game, visibility is driven with Show()).
    bool toastSuccessVisible = false;
    bool toastErrorVisible = false;
    bool previewInsurancePanel = false;
    bool previewSharePanel = false;
    bool previewSpotBar = false;

    // Widgets (auto-bound by data-name).
    Widget garageRoot;
    Widget spotBar;
    Widget adminMenuBtn;
    Widget nearbyCard;
    Widget nearbySelectedBg;
    Widget nearbyChipOk;
    Widget nearbyChipBlocked;
    Widget nearbyReason;
    Widget nearbyParkBtn;
    Widget nearbyEmpty;
    Widget slotsBar;
    Widget listEmpty;
    Widget previewArea;
    Widget previewPlaceholderText;
    Widget viewAtSpotBtn;
    Widget tilesGrid;
    Widget tileBodyBar;
    Widget tileEngineBar;
    Widget tileFuelBar;
    Widget tilePartsBar;
    Widget primaryBtn;
    Widget primaryLabel;
    Widget primaryPrice;
    Widget servicesBar;
    Widget repairBtn;
    Widget insureBtn;
    Widget shareBtn;
    Widget insurancePanel;
    Widget sharePanel;
    Widget shareEmpty;
    Widget spotChipFree;
    Widget spotChipBlocked;
    Widget toastSuccess;
    Widget toastError;

    EntityAI vehiclePreview;
    ItemPreviewWidget vehiclePreviewWidget;

    // The garage NPC the player interacted with (set by TXGUIService before init).
    int m_NpcId = -1;

    protected string m_SelectedId = "";
    protected int    m_InsDays = 1;
    protected bool   m_AdminArmed = false;
    // Title of the toast answering the last request (the server sends the body).
    protected string m_ActionDone = "#txg_garage";
    protected string m_ActionFailed = "#txg_garage";
    protected ref TXGSpotCamera m_SpotCamera;

    // tile bars sit inside their track and are sized relative to it
    static const float TILE_BAR_WIDTH = 1.0;

    override protected void RegisterMetadata()
    {
        m_Metadata = new ComponentMetadata();
        m_Metadata.selector = "txg-garage";
        m_Metadata.templatePath = "TraderXGarage/datasets/layouts/GaragePageView.layout";
    }

    void OnLauncher()
    {
#ifdef TraderX
        TXBeansPromoComponent.Open();
#endif
    }

    override void OnInit()
    {
        vehicles = new array<ref TXGVehicleRowVM>();
        sharePlayers = new array<ref TXGSharePlayerVM>();
        m_InsDays = Math.Max(1, GetTXGConfigService().GetClientInsuranceDefaultDays());
        if (adminMenuBtn)
            adminMenuBtn.Show(GetTXGConfigService().GetClientIsAdmin());
        if (spotBar)
            spotBar.Show(false);
        ShowPanel("");

        TXGGarageService.Event_OnVehiclesUpdated.Insert(OnVehiclesUpdated);
        TXGGarageService.Event_OnNotify.Insert(OnNotify);
        TXGVehicleRowComponent.Event_OnVehicleSelected.Insert(OnVehicleSelected);
        TXGShareRowComponent.Event_OnShareChange.Insert(OnShareChange);

        HideToast();
        OnVehiclesUpdated();
    }

    override void OnDestroy()
    {
        TXGGarageService.Event_OnVehiclesUpdated.Remove(OnVehiclesUpdated);
        TXGGarageService.Event_OnNotify.Remove(OnNotify);
        TXGVehicleRowComponent.Event_OnVehicleSelected.Remove(OnVehicleSelected);
        TXGShareRowComponent.Event_OnShareChange.Remove(OnShareChange);
        GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(HideToast);
        GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(DisarmAdmin);
        StopSpotCamera();
        ClearVehiclePreview();
    }

    // ================================================================== data

    void OnVehiclesUpdated()
    {
        TXGGarageService svc = GetTXGGarageService();
        vehicles = new array<ref TXGVehicleRowVM>();
        array<ref TXGVehicleRowVM> cached = svc.GetClientVehicles();
        if (cached)
        {
            foreach (TXGVehicleRowVM v : cached)
            {
                if (v)
                    vehicles.Insert(v);
            }
        }

        int stored = 0;
        foreach (TXGVehicleRowVM row : vehicles)
        {
            if (!row.isShared)
                stored++;
        }
        int maxSlots = Math.Max(1, svc.GetClientMaxSlots());
        vehicleCountDisplay = stored.ToString() + " / " + maxSlots.ToString();
        float slotsFraction = stored;
        slotsFraction = slotsFraction / maxSlots;
        SetBarFraction(slotsBar, slotsFraction, 1.0);
        if (listEmpty)
            listEmpty.Show(vehicles.Count() == 0);

        balanceText = BuildBalanceText();
        RefreshNearbyCard();

        TXGVehicleRowVM selected = FindRow(m_SelectedId);
        if (!selected)
        {
            m_SelectedId = "";
            if (svc.GetClientNearby())
                m_SelectedId = TXGConstants.NEARBY_VEHICLE_ID;
            else if (vehicles.Count() > 0)
                m_SelectedId = vehicles[0].vehicleId;
            Select(FindRow(m_SelectedId));
        }
        else
        {
            ApplyDetails(selected);
            if (sharePanel && sharePanel.IsVisible())
                BuildShareList(selected);
        }
        MarkForCheck();
    }

    protected string BuildBalanceText()
    {
        TXGGarageService svc = GetTXGGarageService();
        string text = TXGText.Get("#txg_balance_cash", FormatMoney(svc.GetClientCash()));
        if (svc.GetClientBank() >= 0)
            text = TXGText.Get("#txg_balance_bank", FormatMoney(svc.GetClientBank()), text);
        return text;
    }

    protected int AvailableMoney()
    {
        TXGGarageService svc = GetTXGGarageService();
        int total = svc.GetClientCash();
        if (svc.GetClientBank() > 0)
            total = total + svc.GetClientBank();
        return total;
    }

    protected void RefreshNearbyCard()
    {
        TXGVehicleRowVM nearby = GetTXGGarageService().GetClientNearby();
        bool has = (nearby != null);
        if (nearbyCard) nearbyCard.Show(has);
        if (nearbyEmpty) nearbyEmpty.Show(!has);
        if (!has)
            return;

        bool canPark = (nearby.parkInBlockedReason == "");
        nearbyNameText = nearby.displayName;
        nearbyStateText = TXGText.Get("#txg_row_state", TXGText.Localize(nearby.healthText), nearby.fuelPercent.ToString(), nearby.attachmentClasses.Count().ToString());
        nearbyStatusText = nearby.parkInBlockedReason;
        nearbyParkText = TXGText.Get("#txg_park_in_cost", FormatMoney(GetTXGConfigService().GetClientParkInCost()));
        if (nearbyChipOk) nearbyChipOk.Show(canPark);
        if (nearbyChipBlocked) nearbyChipBlocked.Show(!canPark);
        if (nearbyReason) nearbyReason.Show(!canPark);
        if (nearbyParkBtn) nearbyParkBtn.Show(canPark);
    }

    // ================================================================== selection

    void OnSelectNearby()
    {
        Select(FindRow(TXGConstants.NEARBY_VEHICLE_ID));
    }

    void OnVehicleSelected(string vehicleId)
    {
        Select(FindRow(vehicleId));
    }

    protected void Select(TXGVehicleRowVM row)
    {
        m_SelectedId = "";
        if (row)
            m_SelectedId = row.vehicleId;
        TXGVehicleRowComponent.SetSelectedId(m_SelectedId);
        if (nearbySelectedBg)
            nearbySelectedBg.Show(m_SelectedId == TXGConstants.NEARBY_VEHICLE_ID);

        ShowPanel("");
        m_InsDays = Math.Max(1, GetTXGConfigService().GetClientInsuranceDefaultDays());
        ApplyDetails(row);
        if (row)
            SetVehiclePreview(row.className, row.attachmentClasses);
        else
            ClearVehiclePreview();
        MarkForCheck();
    }

    protected void ApplyDetails(TXGVehicleRowVM row)
    {
        TXGConfigService cfg = GetTXGConfigService();
        bool hasRow = (row != null);
        bool isStored = false;
        if (hasRow)
            isStored = !row.isNearby;

        if (primaryBtn) primaryBtn.Show(hasRow);
        if (primaryLabel) primaryLabel.Show(hasRow);
        if (primaryPrice) primaryPrice.Show(hasRow);
        if (servicesBar) servicesBar.Show(isStored);
        if (viewAtSpotBtn) viewAtSpotBtn.Show(isStored);
        if (tilesGrid) tilesGrid.Show(hasRow);

        if (!hasRow)
        {
            selectedVehicleName = "";
            selectedBadgesText = "";
            servicesHintText = "";
            return;
        }

        selectedVehicleName = row.displayName;
        selectedBadgesText = BuildBadges(row);
        tileBodyText = TXGText.Localize(TXGGarageService.GetHealthLevelText(row.chassisHealthLevel));
        tileEngineText = TXGText.Localize(TXGGarageService.GetHealthLevelText(row.engineHealthLevel));
        tileFuelText = row.fuelPercent.ToString() + " %";
        tilePartsText = row.attachmentClasses.Count().ToString() + " / " + row.cargoCount.ToString();
        SetTileBar(tileBodyBar, HealthFraction(row.chassisHealthLevel));
        SetTileBar(tileEngineBar, HealthFraction(row.engineHealthLevel));
        float fuelFraction = row.fuelPercent;
        SetTileBar(tileFuelBar, fuelFraction / 100.0);
        SetTileBar(tilePartsBar, 1.0);

        int price = 0;
        if (row.isNearby)
        {
            primaryText = "#txg_park_in";
            price = cfg.GetClientParkInCost();
            if (row.parkInBlockedReason != "")
                primaryText = row.parkInBlockedReason;
        }
        else if (row.holdReason != "" && row.releaseFee > 0)
        {
            primaryText = "#txg_take_out_replacement";
            if (row.holdReason == TXGConstants.HOLD_IMPOUND)
                primaryText = "#txg_release_impound";
            price = row.releaseFee;
        }
        else
        {
            primaryText = "#txg_take_out";
            price = cfg.GetClientParkOutCost();
        }
        primaryPriceText = FormatMoney(price);

        bool canAfford = price <= AvailableMoney();
        float alpha = 1.0;
        if (!canAfford || (row.isNearby && row.parkInBlockedReason != ""))
            alpha = 0.45;
        if (primaryLabel) primaryLabel.SetAlpha(alpha);
        if (primaryPrice) primaryPrice.SetAlpha(alpha);

        servicesHintText = "";
        if (!canAfford)
            servicesHintText = TXGText.Get("#txg_hint_no_money", FormatMoney(price), FormatMoney(AvailableMoney()));
        else if (row.isShared)
            servicesHintText = "#txg_hint_shared";
        else if (isStored)
            servicesHintText = "#txg_hint_bank_first";

        if (!isStored)
            return;

        bool repairs = cfg.GetClientEnableRepairs() && !row.isShared;
        bool insurance = cfg.GetClientEnableInsurance() && !row.isShared && row.holdReason == "" && row.insurancePerDay > 0;
        if (repairBtn) repairBtn.Show(repairs);
        if (insureBtn) insureBtn.Show(insurance);
        if (shareBtn) shareBtn.Show(!row.isShared);

        if (row.repairQuote > 0)
            repairBtnText = TXGText.Get("#txg_service_cost", FormatMoney(row.repairQuote));
        else
            repairBtnText = "#txg_service_ok";

        if (row.isInsured)
            insureBtnText = TXGText.Get("#txg_insurance_days_left", DaysLeft(row).ToString());
        else
            insureBtnText = "#txg_insurance";

        if (row.sharedWith.Count() > 0)
            shareBtnText = TXGText.Get("#txg_share_access_n", row.sharedWith.Count().ToString());
        else
            shareBtnText = "#txg_share_access";
    }

    protected string BuildBadges(TXGVehicleRowVM row)
    {
        string text = "";
        if (row.holdReason != "")
            text = AddBadge(text, TXGText.Get("#txg_badge_hold", HoldLabel(row.holdReason)));
        if (row.isShared)
            text = AddBadge(text, TXGText.Localize("#txg_badge_shared_with_you"));
        if (row.isInsured)
            text = AddBadge(text, TXGText.Get("#txg_badge_insured", DaysLeft(row).ToString()));
        if (row.hasCarLock)
            text = AddBadge(text, TXGText.Localize("#txg_badge_carlock"));
        if (row.sharedWith.Count() > 0)
            text = AddBadge(text, TXGText.Get("#txg_badge_shared_n", row.sharedWith.Count().ToString()));
        return text;
    }

    // "INSURANCE" / "IMPOUND" hold constants as player text
    static string HoldLabel(string holdReason)
    {
        if (holdReason == TXGConstants.HOLD_IMPOUND)
            return TXGText.Localize("#txg_hold_impound");
        if (holdReason == TXGConstants.HOLD_INSURANCE)
            return TXGText.Localize("#txg_hold_insurance");
        return holdReason;
    }

    protected string AddBadge(string text, string badge)
    {
        if (text == "")
            return badge;
        return text + "   " + badge;
    }

    protected int DaysLeft(TXGVehicleRowVM row)
    {
        if (!row.isInsured || row.insuranceExpiresAt <= 0)
            return 0;
        int seconds = row.insuranceExpiresAt - GetTXGGarageService().GetClientNowUnix();
        if (seconds <= 0)
            return 0;
        return (seconds + 86399) / 86400;
    }

    // ================================================================== panels

    protected void ShowPanel(string panel)
    {
        bool ins = (panel == "insurance");
        bool share = (panel == "share");
        if (insurancePanel) insurancePanel.Show(ins);
        if (sharePanel) sharePanel.Show(share);
        if (previewArea) previewArea.Show(!ins && !share);
        bool primary = !ins && !share && m_SelectedId != "";
        if (primaryBtn) primaryBtn.Show(primary);
        if (primaryLabel) primaryLabel.Show(primary);
        if (primaryPrice) primaryPrice.Show(primary);
        if (tilesGrid) tilesGrid.Show(primary);
    }

    void OnClosePanels()
    {
        ShowPanel("");
        MarkForCheck();
    }

    void OnOpenInsurance()
    {
        TXGVehicleRowVM row = FindRow(m_SelectedId);
        if (!row || row.isNearby)
            return;
        ShowPanel("insurance");
        RefreshInsurance(row);
        MarkForCheck();
    }

    protected void RefreshInsurance(TXGVehicleRowVM row)
    {
        int maxDays = Math.Max(1, GetTXGConfigService().GetClientInsuranceMaxDays());
        m_InsDays = Math.Clamp(m_InsDays, 1, maxDays);
        int total = row.insurancePerDay * m_InsDays;
        int left = DaysLeft(row);

        if (row.isInsured)
            insStatusText = TXGText.Get("#txg_ins_covered_for", left.ToString());
        else
            insStatusText = "#txg_ins_not_insured";
        insCoveredText = FormatMoney(row.insuranceCoveredValue);
        insPerDayText = FormatMoney(row.insurancePerDay);
        insDeductibleText = FormatMoney(row.insuranceDeductibleQuote);
        insDaysText = TXGText.Get("#txg_ins_day", m_InsDays.ToString());
        if (m_InsDays > 1)
            insDaysText = TXGText.Get("#txg_ins_days", m_InsDays.ToString());
        insUntilText = TXGText.Get("#txg_ins_until", (left + m_InsDays).ToString());
        if (row.isInsured)
            insConfirmText = TXGText.Get("#txg_ins_extend", insDaysText);
        else
            insConfirmText = TXGText.Get("#txg_ins_insure_for", insDaysText);
        insTotalText = FormatMoney(total);
    }

    void OnInsDaysMinus()
    {
        ChangeInsDays(-1);
    }

    void OnInsDaysPlus()
    {
        ChangeInsDays(1);
    }

    protected void ChangeInsDays(int delta)
    {
        TXGVehicleRowVM row = FindRow(m_SelectedId);
        if (!row)
            return;
        m_InsDays = m_InsDays + delta;
        RefreshInsurance(row);
        MarkForCheck();
    }

    void OnOpenShare()
    {
        TXGVehicleRowVM row = FindRow(m_SelectedId);
        if (!row || row.isNearby || row.isShared)
            return;
        ShowPanel("share");
        BuildShareList(row);
        MarkForCheck();
    }

    protected void BuildShareList(TXGVehicleRowVM row)
    {
        sharePlayers = new array<ref TXGSharePlayerVM>();
        foreach (TXGSharePlayerVM p : GetTXGGarageService().GetClientPeople())
        {
            TXGSharePlayerVM item = new TXGSharePlayerVM();
            item.uid = p.uid;
            item.name = p.name;
            item.distance = p.distance;
            item.vehicleId = row.vehicleId;
            item.hasAccess = (row.sharedWith.Find(p.uid) != -1);
            sharePlayers.Insert(item);
        }
        if (shareEmpty)
            shareEmpty.Show(sharePlayers.Count() == 0);
    }

    void OnShareChange(string vehicleId, string uid, bool add)
    {
        int mode = 2;
        if (add)
            mode = 1;
        ExpectResult("#txg_t_shared_access", "#txg_t_sharing_failed");
        TXGVehicleActionRequest req = new TXGVehicleActionRequest(vehicleId, mode, uid);
        GetDayZularRPCManager().SendRPC(TXGConstants.RPC_NAMESPACE, "RequestShare", new Param1<ref TXGVehicleActionRequest>(req), true, null);
    }

    // ================================================================== toasts

    protected void ExpectResult(string done, string failed)
    {
        m_ActionDone = done;
        m_ActionFailed = failed;
    }

    // Server answers go through the TraderX toast cards when TraderX is loaded, so the garage reads
    // like the rest of the trader; standalone falls back to the garage's own banner.
    void OnNotify(string message, bool success)
    {
#ifdef TraderX
        TraderXToastService toasts = GetTraderXToastService();
        if (toasts)
        {
            string title = m_ActionFailed;
            string kind = TXNOTIF_ERROR;
            if (success)
            {
                title = m_ActionDone;
                kind = TXNOTIF_INFO;
            }
            toasts.ShowToast(kind, TXGText.Localize(title), TXGText.Localize(message), TXGText.Localize("#txg_garage"));
            ExpectResult("#txg_garage", "#txg_garage");
            return;
        }
#endif
        toastText = TXGText.Localize(message);
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

    // ================================================================== preview + camera

    protected void SetVehiclePreview(string className, array<string> attachments)
    {
        ClearVehiclePreview();
        if (className == "")
            return;

        // The hologram on the parking spot is only meaningful for a vehicle that would be taken out.
        TXGPreviewService previewSvc = GetTXGPreviewService();
        previewSvc.SetSpotPreviewEnabled(m_SelectedId != TXGConstants.NEARBY_VEHICLE_ID);
        vehiclePreview = previewSvc.UpdatePreview(className, attachments, m_NpcId);
        if (previewPlaceholderText)
            previewPlaceholderText.Show(vehiclePreview == null);
        MarkForCheck();
    }

    protected void ClearVehiclePreview()
    {
        TXGPreviewService previewSvc = GetTXGPreviewService();
        if (previewSvc)
            previewSvc.ClearAll();
        vehiclePreview = null;
        if (vehiclePreviewWidget)
            vehiclePreviewWidget.SetItem(null);
        if (previewPlaceholderText)
            previewPlaceholderText.Show(true);
    }

    void OnViewAtSpot()
    {
        vector spotPos;
        vector spotOri;
        vector spotSize;
        Man player = GetGame().GetPlayer();
        if (!player || !GetTXGConfigService().ResolveClientParkSpotEx(m_NpcId, player.GetPosition(), spotPos, spotOri, spotSize))
        {
            ExpectResult("#txg_garage", "#txg_t_parking_spot");
            OnNotify("#txg_err_no_spot_configured", false);
            return;
        }

        if (!m_SpotCamera)
            m_SpotCamera = new TXGSpotCamera();
        if (!m_SpotCamera.Start(spotPos, spotOri))
        {
            ExpectResult("#txg_garage", "#txg_t_parking_spot");
            OnNotify("#txg_err_camera", false);
            return;
        }

        bool free = IsSpotFree(spotPos, spotOri, spotSize);
        if (spotChipFree) spotChipFree.Show(free);
        if (spotChipBlocked) spotChipBlocked.Show(!free);
        if (garageRoot) garageRoot.Show(false);
        if (spotBar) spotBar.Show(true);
        MarkForCheck();
    }

    // Client-side check of the spot box, ignoring our own hologram.
    protected bool IsSpotFree(vector pos, vector ori, vector size)
    {
        if (size == vector.Zero)
            return true;
        array<Object> excluded = new array<Object>();
        EntityAI hologram = GetTXGPreviewService().GetSpotPreview();
        if (hologram)
            excluded.Insert(hologram);
        array<Object> hits = new array<Object>();
        GetGame().IsBoxColliding(pos, ori, size, excluded, hits);
        return hits.Count() == 0;
    }

    void OnExitSpotView()
    {
        StopSpotCamera();
        if (spotBar) spotBar.Show(false);
        if (garageRoot) garageRoot.Show(true);
        MarkForCheck();
    }

    protected void StopSpotCamera()
    {
        if (m_SpotCamera)
            m_SpotCamera.Stop();
    }

    bool IsSpotView()
    {
        if (!m_SpotCamera)
            return false;
        return m_SpotCamera.IsActive();
    }

    void OnSpotRotateLeft()  { if (m_SpotCamera) m_SpotCamera.Rotate(-20); }
    void OnSpotRotateRight() { if (m_SpotCamera) m_SpotCamera.Rotate(20); }
    void OnSpotZoomIn()      { if (m_SpotCamera) m_SpotCamera.Zoom(-1.5); }
    void OnSpotZoomOut()     { if (m_SpotCamera) m_SpotCamera.Zoom(1.5); }

    // Keyboard while the camera view is active (routed from MissionGameplay). True = handled.
    bool HandleSpotKey(int key)
    {
        if (!IsSpotView())
            return false;
        if (key == KeyCode.KC_A || key == KeyCode.KC_LEFT)
            OnSpotRotateLeft();
        else if (key == KeyCode.KC_D || key == KeyCode.KC_RIGHT)
            OnSpotRotateRight();
        else if (key == KeyCode.KC_W || key == KeyCode.KC_UP)
            OnSpotZoomIn();
        else if (key == KeyCode.KC_S || key == KeyCode.KC_DOWN)
            OnSpotZoomOut();
        else
            return false;
        return true;
    }

    // ================================================================== actions

    void OnPrimary()
    {
        TXGVehicleRowVM row = FindRow(m_SelectedId);
        if (!row)
            return;

        if (row.isNearby)
        {
            OnParkNearby();
            return;
        }

        ExpectResult("#txg_t_taken_out", "#txg_t_cannot_take_out");
        if (IsSpotView())
            OnExitSpotView();
        TXGParkOutRequest req = new TXGParkOutRequest(m_SelectedId, m_NpcId);
        GetDayZularRPCManager().SendRPC(TXGConstants.RPC_NAMESPACE, "RequestParkOut", new Param1<ref TXGParkOutRequest>(req), true, null);
    }

    void OnParkNearby()
    {
        TXGVehicleRowVM nearby = GetTXGGarageService().GetClientNearby();
        if (!nearby)
            return;
        if (nearby.parkInBlockedReason != "")
        {
            ExpectResult("#txg_garage", "#txg_t_cannot_park");
            OnNotify(nearby.parkInBlockedReason, false);
            return;
        }
        ExpectResult("#txg_msg_parked", "#txg_t_cannot_park");
        GetDayZularRPCManager().SendRPC(TXGConstants.RPC_NAMESPACE, "RequestParkIn", null, true, null);
    }

    void OnRepair()
    {
        if (!IsStoredSelected())
            return;
        ExpectResult("#txg_t_serviced", "#txg_t_service_failed");
        TXGVehicleActionRequest req = new TXGVehicleActionRequest(m_SelectedId, 0, "");
        GetDayZularRPCManager().SendRPC(TXGConstants.RPC_NAMESPACE, "RequestRepair", new Param1<ref TXGVehicleActionRequest>(req), true, null);
    }

    void OnInsure()
    {
        if (!IsStoredSelected())
            return;
        ExpectResult("#txg_t_insurance", "#txg_t_insurance_refused");
        TXGVehicleActionRequest req = new TXGVehicleActionRequest(m_SelectedId, m_InsDays, "");
        GetDayZularRPCManager().SendRPC(TXGConstants.RPC_NAMESPACE, "RequestInsure", new Param1<ref TXGVehicleActionRequest>(req), true, null);
        ShowPanel("");
    }

    // Admin menu: first click arms, second click (within 3 s) impounds the nearest vehicle.
    void OnAdminMenu()
    {
        if (!m_AdminArmed)
        {
            m_AdminArmed = true;
            ExpectResult("#txg_t_admin", "#txg_t_admin");
            OnNotify("#txg_admin_hint", false);
            GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(DisarmAdmin, 3000, false);
            return;
        }
        DisarmAdmin();
        ExpectResult("#txg_t_admin_impound", "#txg_t_impound_failed");
        GetDayZularRPCManager().SendRPC(TXGConstants.RPC_NAMESPACE, "RequestAdminImpound", null, true, null);
    }

    protected void DisarmAdmin()
    {
        m_AdminArmed = false;
    }

    void OnClose()
    {
        TXGUIService svc = GetTXGUIService();
        if (svc)
            svc.CloseGarageMenu();
    }

    // ================================================================== helpers

    protected bool IsStoredSelected()
    {
        TXGVehicleRowVM row = FindRow(m_SelectedId);
        if (!row)
            return false;
        return !row.isNearby;
    }

    protected TXGVehicleRowVM FindRow(string vehicleId)
    {
        if (vehicleId == "")
            return null;
        if (vehicleId == TXGConstants.NEARBY_VEHICLE_ID)
            return GetTXGGarageService().GetClientNearby();
        foreach (TXGVehicleRowVM v : vehicles)
        {
            if (v && v.vehicleId == vehicleId)
                return v;
        }
        return null;
    }

    protected void SetTileBar(Widget bar, float fraction)
    {
        SetBarFraction(bar, fraction, TILE_BAR_WIDTH);
        if (!bar)
            return;
        // Nasdara Good / Gold / Bad
        if (fraction > 0.66)
            bar.SetColor(ARGB(255, 127, 166, 90));
        else if (fraction > 0.33)
            bar.SetColor(ARGB(255, 224, 163, 58));
        else
            bar.SetColor(ARGB(255, 224, 122, 99));
    }

    // Bars are laid out in relative units: keep the height, scale the width.
    protected void SetBarFraction(Widget bar, float fraction, float fullWidth)
    {
        if (!bar)
            return;
        float w;
        float h;
        bar.GetSize(w, h);
        bar.SetSize(fullWidth * Math.Clamp(fraction, 0.0, 1.0), h);
    }

    protected static float HealthFraction(int level)
    {
        if (level == TXGConstants.HEALTH_PRISTINE)
            return 1.0;
        if (level == TXGConstants.HEALTH_WORN)
            return 0.7;
        if (level == TXGConstants.HEALTH_DAMAGED)
            return 0.5;
        if (level == TXGConstants.HEALTH_BADLY_DAMAGED)
            return 0.3;
        return 0.0;
    }

    // 12400 -> "12 400"
    static string FormatMoney(int amount)
    {
        string digits = Math.AbsInt(amount).ToString();
        string result = "";
        int count = 0;
        for (int i = digits.Length() - 1; i >= 0; i--)
        {
            if (count > 0 && count % 3 == 0)
                result = " " + result;
            result = digits.Get(i) + result;
            count++;
        }
        if (amount < 0)
            result = "-" + result;
        return result;
    }
}
