// TraderXGarage - GarageVehicleCardComponent
// Vehicle list item component. Displays a single vehicle entry with selection support.

class GarageVehicleCardComponent extends DZlrComponentBase
{
	private static ref ComponentAutoRegister s_Register =
		new ComponentAutoRegister("txg-garage-vehicle-card", GarageVehicleCardComponent);

	static ref ScriptInvoker Event_OnVehicleSelected;

	// ─── Input Binding ───
	ref GarageVehicleViewData _inputVehicle;
	string vehicleDisplayName;

	// ─── Widget References ───
	ButtonWidget VehicleButton;

	override protected void RegisterMetadata()
	{
		m_Metadata = new ComponentMetadata();
		m_Metadata.selector = "txg-garage-vehicle-card";
		m_Metadata.templatePath = "TraderXGarage/datasets/layouts/GarageVehicleCardComponent.layout";
	}

	override void OnInit()
	{
		if (_inputVehicle)
		{
			vehicleDisplayName = _inputVehicle.vehicleName;

			// Highlight parked-at-spot vehicles in red
			if (VehicleButton && _inputVehicle.isParkedAtSpot)
				VehicleButton.SetColor(ARGB(255, 191, 48, 48));
		}

		MarkForCheck();
	}

	void OnVehicleClicked(ClickEventArgs args)
	{
		if (_inputVehicle && Event_OnVehicleSelected)
			Event_OnVehicleSelected.Invoke(_inputVehicle);
	}
}
