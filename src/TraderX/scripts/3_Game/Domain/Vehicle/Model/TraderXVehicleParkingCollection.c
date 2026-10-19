class TraderXVehicleParkingCollection
{
    string traderId;
    ref array<ref TraderXVehicleParkingPosition> positions;

    void TraderXVehicleParkingCollection(string traderId)
    {
        this.traderId = traderId;
        this.positions = new array<ref TraderXVehicleParkingPosition>;
    }
}