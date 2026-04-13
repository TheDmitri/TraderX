// TraderXGarage - GarageVehicleViewData
// UI-level data model for vehicle list display in the garage component.

class GarageVehicleViewData
{
	string vehicleName;
	bool isParkedAtSpot;
	int index;

	void GarageVehicleViewData(string name = "", bool parked = false, int idx = 0)
	{
		vehicleName = name;
		isParkedAtSpot = parked;
		index = idx;
	}
}
