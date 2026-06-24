class TXSafeZoneLocation
{   
    int id;
    bool isActive;
    string name;
    vector position;
    int radius;
    bool isEntitiesCleanActive = true;
    ref array<string> allowedAnimals;

    void TXSafeZoneLocation(bool isActive, int id, string name, vector position, int radius, bool isEntitiesCleanActive, array<string> allowedAnimals)
    {
        this.isActive = isActive;
        this.id = id;
        this.name = name;
        this.position = position;
        this.radius = radius;
        this.isEntitiesCleanActive = isEntitiesCleanActive;
        this.allowedAnimals = allowedAnimals;
    }
}
