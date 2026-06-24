// TraderXGarage - Park spot
// One vehicle slot belonging to a garage NPC. Multiple spots per NPC support several
// stored vehicles parked out at once; `size` drives the collision check (zero = no check).

class TXGParkSpot
{
    vector position;
    vector orientation;
    vector size;        // collision box (x y z). vector.Zero = skip collision test.

    void TXGParkSpot(vector _position = vector.Zero, vector _orientation = vector.Zero, vector _size = vector.Zero)
    {
        position = _position;
        orientation = _orientation;
        size = _size;
    }
}
