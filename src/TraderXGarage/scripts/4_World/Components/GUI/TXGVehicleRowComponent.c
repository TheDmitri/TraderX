// TraderXGarage - Vehicle Row Component
// ForEach row template. One instance per stored vehicle (TXGVehicleRowVM).
// Clicking the row publishes Event_OnVehicleSelected with the vehicle id.

class TXGVehicleRowComponent extends DZlrComponentBase
{
    private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("txg-vehicle-row", TXGVehicleRowComponent);

    static ref ScriptInvoker Event_OnVehicleSelected = new ScriptInvoker();
    // Fired with the selected id so every row can show / hide its selection marker.
    static ref ScriptInvoker Event_OnSelectionChanged = new ScriptInvoker();
    protected static string s_SelectedId = "";

    ref TXGVehicleRowVM _inputVehicle;

    string rowName = "-";
    string rowHealth = "-";
    string rowBadgeText = "";

    Widget rowSelectedBg;
    Widget rowHealthBar;

    override protected void RegisterMetadata()
    {
        m_Metadata = new ComponentMetadata();
        m_Metadata.selector = "txg-vehicle-row";
        m_Metadata.templatePath = "TraderXGarage/datasets/layouts/GarageVehicleRow.layout";
    }

    static void SetSelectedId(string vehicleId)
    {
        s_SelectedId = vehicleId;
        Event_OnSelectionChanged.Invoke(vehicleId);
    }

    override void OnInit()
    {
        Event_OnSelectionChanged.Insert(OnSelectionChanged);
        if (!_inputVehicle)
        {
            MarkForCheck();
            return;
        }

        rowName = _inputVehicle.displayName;
        rowHealth = _inputVehicle.healthText + " - Fuel " + _inputVehicle.fuelPercent.ToString() + "% - " + _inputVehicle.attachmentClasses.Count().ToString() + " parts";
        if (_inputVehicle.holdReason != "" && _inputVehicle.releaseFee > 0)
            rowHealth = "Waiting - take out for " + _inputVehicle.releaseFee.ToString();

        if (_inputVehicle.holdReason != "")
            rowBadgeText = _inputVehicle.holdReason;
        else if (_inputVehicle.isShared)
            rowBadgeText = "SHARED";
        else if (_inputVehicle.isInsured)
            rowBadgeText = "INSURED " + _inputVehicle.insuranceDaysLeft.ToString() + "D";
        else if (_inputVehicle.hasCarLock)
            rowBadgeText = "CARLOCK";

        if (rowHealthBar)
            rowHealthBar.SetColor(HealthColor(_inputVehicle.healthLevel));
        OnSelectionChanged(s_SelectedId);
        MarkForCheck();
    }

    override void OnDestroy()
    {
        Event_OnSelectionChanged.Remove(OnSelectionChanged);
    }

    void OnSelectionChanged(string vehicleId)
    {
        if (rowSelectedBg && _inputVehicle)
            rowSelectedBg.Show(_inputVehicle.vehicleId == vehicleId);
    }

    protected static int HealthColor(int level)
    {
        if (level == TXGConstants.HEALTH_PRISTINE)
            return ARGB(255, 122, 154, 74);
        if (level == TXGConstants.HEALTH_WORN)
            return ARGB(255, 154, 176, 90);
        if (level == TXGConstants.HEALTH_DAMAGED)
            return ARGB(255, 212, 164, 76);
        if (level == TXGConstants.HEALTH_BADLY_DAMAGED)
            return ARGB(255, 200, 102, 74);
        return ARGB(255, 201, 48, 48);
    }

    void OnVehicleSelected()
    {
        if (!_inputVehicle)
            return;
        Event_OnVehicleSelected.Invoke(_inputVehicle.vehicleId);
    }
}
