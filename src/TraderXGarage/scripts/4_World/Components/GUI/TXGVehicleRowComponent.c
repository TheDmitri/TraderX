// TraderXGarage - Vehicle Row Component
// ForEach row template. One instance per stored vehicle (TXGVehicleRowVM).
// Clicking the row publishes Event_OnVehicleSelected with the vehicle id.

class TXGVehicleRowComponent extends DZlrComponentBase
{
    private static ref ComponentAutoRegister s_Register = new ComponentAutoRegister("txg-vehicle-row", TXGVehicleRowComponent);

    static ref ScriptInvoker Event_OnVehicleSelected = new ScriptInvoker();

    // Input bound by the ForEach.
    ref TXGVehicleRowVM _inputVehicle;

    // Display bindings.
    string rowName = "—";
    string rowHealth = "—";

    override protected void RegisterMetadata()
    {
        m_Metadata = new ComponentMetadata();
        m_Metadata.selector = "txg-vehicle-row";
        m_Metadata.templatePath = "TraderXGarage/datasets/layouts/GarageVehicleRow.layout";
    }

    override void OnInit()
    {
        if (!_inputVehicle)
        {
            rowName = "—";
            rowHealth = "—";
            MarkForCheck();
            return;
        }

        rowName = _inputVehicle.displayName;
        rowHealth = _inputVehicle.healthText;
        MarkForCheck();
    }

    void OnVehicleSelected()
    {
        if (!_inputVehicle)
            return;
        Event_OnVehicleSelected.Invoke(_inputVehicle.vehicleId);
    }
}
