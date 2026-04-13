// TraderXGarage - CarScript Extension
// Modded CarScript to expose fuel amount for garage serialization.

modded class CarScript
{
	float TXGarageGetFuelAmount()
	{
		return GetFluidFraction(CarFluid.FUEL) * GetFluidCapacity(CarFluid.FUEL);
	}

	void TXGarageSetOrientation(vector orient)
	{
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(SetOrientation, 500, false, orient);
	}
}
