class TraderXVehicleParkingCollection
{
    string traderId;
    ref array<TraderXVehicleParkingPosition> positions;

    void TraderXVehicleParkingCollection(string traderId)
    {
        this.traderId = traderId;
        this.positions = new array<TraderXVehicleParkingPosition>;
    }
}