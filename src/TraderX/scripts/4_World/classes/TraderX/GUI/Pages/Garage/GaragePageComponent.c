/**
 * Garage page component for the TraderX main menu.
 * This is a thin wrapper that delegates all garage operations to the
 * ITraderXGarageProvider. It only renders UI — no garage logic lives here.
 *
 * Only visible when a garage provider is registered and available at the current NPC.
 */
class GaragePageComponent extends DZlrComponentBase
{
    private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("traderx-garage-page", GaragePageComponent);

    // Vehicle row widgets (max 3 rows from layout template, dynamically populated)
    private ref array<ButtonWidget> m_VehicleRowBtns = new array<ButtonWidget>();
    private ref TStringArray m_VehicleNames;
    private ref TIntArray m_VehicleHealths;
    private int m_SelectedIndex = -1;

    // Widgets
    TextWidget balanceValue, listHeaderLabel, previewTitle;
    TextWidget parkInCostLabel, parkOutCostLabel, parkingStatusLabel;
    ButtonWidget parkInBtn, parkOutBtn, leaveBtn;
    Widget vehicleListGrid, garageStatusPanel;
    RichTextWidgetClass garageStatusMessage;
    ItemPreviewWidgetClass vehiclePreview;
    PanelWidgetClass parkingIndicator;

    override protected void RegisterMetadata()
    {
        m_Metadata = new ComponentMetadata();
        m_Metadata.selector = "traderx-garage-page";
        m_Metadata.templatePath = "TraderX/datasets/gui/Garage/GaragePageView.layout";
    }

    override void OnInit()
    {
        ITraderXGarageProvider provider = TraderXProviderRegistry.GetInstance().GetGarageProvider();
        if (!provider)
            return;

        // Subscribe to garage events
        if (provider.GetOnVehicleListReceived())
            provider.GetOnVehicleListReceived().Insert(OnVehicleListReceived);

        if (provider.GetOnParkOutResult())
            provider.GetOnParkOutResult().Insert(OnParkOutResult);

        // Update costs display
        if (parkInCostLabel)
            parkInCostLabel.SetText("IN: " + provider.GetParkInCost().ToString());
        if (parkOutCostLabel)
            parkOutCostLabel.SetText("OUT: " + provider.GetParkOutCost().ToString());

        // Update balance
        UpdateBalance();

        // Request vehicle list from server
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (player)
        {
            int lowId, highId;
            player.GetNetworkID(lowId, highId);

            int npcId = TraderXTradingService.GetInstance().GetNpcId();
            TraderXNpc npc = GetTraderXModule().GetSettings().GetNpcById(npcId);
            vector npcPos = "0 0 0";
            if (npc)
                npcPos = npc.GetPosition();

            provider.RequestVehicleList(lowId, npcPos);
        }

        // Collect vehicle row buttons from layout
        CollectVehicleRows();
    }

    override void OnDestroy()
    {
        ITraderXGarageProvider provider = TraderXProviderRegistry.GetInstance().GetGarageProvider();
        if (provider)
        {
            if (provider.GetOnVehicleListReceived())
                provider.GetOnVehicleListReceived().Remove(OnVehicleListReceived);
            if (provider.GetOnParkOutResult())
                provider.GetOnParkOutResult().Remove(OnParkOutResult);
        }
    }

    private void CollectVehicleRows()
    {
        m_VehicleRowBtns.Clear();

        if (!m_Root)
            return;

        // Collect the template vehicle row buttons from the layout
        ButtonWidget row0 = ButtonWidget.Cast(m_Root.FindAnyWidget("vehicleRowBtn"));
        ButtonWidget row1 = ButtonWidget.Cast(m_Root.FindAnyWidget("vehicleRowBtn0"));
        ButtonWidget row2 = ButtonWidget.Cast(m_Root.FindAnyWidget("vehicleRowBtn1"));

        if (row0) m_VehicleRowBtns.Insert(row0);
        if (row1) m_VehicleRowBtns.Insert(row1);
        if (row2) m_VehicleRowBtns.Insert(row2);

        // Hide all rows initially
        foreach (ButtonWidget btn : m_VehicleRowBtns)
        {
            if (btn)
                btn.Show(false);
        }
    }

    private void UpdateBalance()
    {
        if (!balanceValue)
            return;

        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player)
            return;

        int totalMoney = TraderXTradingService.GetInstance().GetPlayerTotalMoneyAmount(player);
        balanceValue.SetText(TraderXQuantityManager.GetFormattedMoneyAmount(totalMoney));
    }

    // --- Event handlers ---

    void OnVehicleListReceived(TStringArray vehicleNames, TIntArray healthValues, int responseCode)
    {
        m_VehicleNames = vehicleNames;
        m_VehicleHealths = healthValues;
        m_SelectedIndex = -1;

        // Update count label
        ITraderXGarageProvider provider = TraderXProviderRegistry.GetInstance().GetGarageProvider();
        GarageService service = GetGarageService();
        int maxSlots = 5;
        if (service && service.GetSettings())
            maxSlots = service.GetSettings().maxVehiclesStored;

        int count = 0;
        if (m_VehicleNames)
            count = m_VehicleNames.Count();

        if (listHeaderLabel)
            listHeaderLabel.SetText(count.ToString() + "/" + maxSlots.ToString());

        // Populate vehicle rows
        for (int i = 0; i < m_VehicleRowBtns.Count(); i++)
        {
            ButtonWidget rowBtn = m_VehicleRowBtns[i];
            if (!rowBtn)
                continue;

            if (i < count)
            {
                rowBtn.Show(true);
                // Find child widgets to set text
                string suffix = "";
                if (i == 1) suffix = "0";
                else if (i == 2) suffix = "1";

                TextWidget nameWidget = TextWidget.Cast(m_Root.FindAnyWidget("vehicleName" + suffix));
                TextWidget healthWidget = TextWidget.Cast(m_Root.FindAnyWidget("vehicleHealth" + suffix));

                if (nameWidget)
                    nameWidget.SetText(m_VehicleNames[i]);

                if (healthWidget && m_VehicleHealths)
                {
                    int health = m_VehicleHealths[i];
                    healthWidget.SetText(GetHealthLabel(health));
                }
            }
            else
            {
                rowBtn.Show(false);
            }
        }
    }

    void OnParkOutResult(bool success, string message)
    {
        if (garageStatusPanel && garageStatusMessage)
        {
            garageStatusPanel.Show(true);
            garageStatusMessage.SetText(message);
        }

        // Refresh the vehicle list
        if (success)
        {
            ITraderXGarageProvider provider = TraderXProviderRegistry.GetInstance().GetGarageProvider();
            if (provider)
            {
                PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
                if (player)
                {
                    int lowId, highId;
                    player.GetNetworkID(lowId, highId);

                    int npcId = TraderXTradingService.GetInstance().GetNpcId();
                    TraderXNpc npc = GetTraderXModule().GetSettings().GetNpcById(npcId);
                    vector npcPos = "0 0 0";
                    if (npc)
                        npcPos = npc.GetPosition();

                    provider.RequestVehicleList(lowId, npcPos);
                }
            }
            UpdateBalance();
        }
    }

    private string GetHealthLabel(int healthLevel)
    {
        switch (healthLevel)
        {
            case 0: return "Pristine";
            case 1: return "Worn";
            case 2: return "Damaged";
            case 3: return "Badly Damaged";
            case 4: return "Ruined";
        }
        return "Unknown";
    }

    // --- Button click handling ---

    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (button != MouseState.LEFT)
            return false;

        if (w == leaveBtn)
        {
            // Close the garage page and go back to main
            TraderXMainComponent mainComp = TraderXMainComponent.GetInstance();
            if (mainComp)
                mainComp.RemoveSubPage();
            return true;
        }

        // Check for vehicle row selection
        for (int i = 0; i < m_VehicleRowBtns.Count(); i++)
        {
            if (w == m_VehicleRowBtns[i])
            {
                SelectVehicle(i);
                return true;
            }
        }

        if (w == parkOutBtn)
        {
            OnParkOutClicked();
            return true;
        }

        return false;
    }

    private void SelectVehicle(int index)
    {
        m_SelectedIndex = index;

        if (previewTitle && m_VehicleNames && index < m_VehicleNames.Count())
            previewTitle.SetText(m_VehicleNames[index]);
    }

    private void OnParkOutClicked()
    {
        if (m_SelectedIndex < 0 || !m_VehicleNames || m_SelectedIndex >= m_VehicleNames.Count())
            return;

        ITraderXGarageProvider provider = TraderXProviderRegistry.GetInstance().GetGarageProvider();
        if (!provider)
            return;

        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player)
            return;

        int lowId, highId;
        player.GetNetworkID(lowId, highId);

        int npcId = TraderXTradingService.GetInstance().GetNpcId();
        TraderXNpc npc = GetTraderXModule().GetSettings().GetNpcById(npcId);
        vector npcPos = "0 0 0";
        if (npc)
            npcPos = npc.GetPosition();

        string vehicleName = m_VehicleNames[m_SelectedIndex];
        provider.RequestParkOut(lowId, m_SelectedIndex, vehicleName, npcPos);
    }
}
