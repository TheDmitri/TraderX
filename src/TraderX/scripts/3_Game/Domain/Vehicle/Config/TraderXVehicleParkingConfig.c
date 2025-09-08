class TraderXVehicleParkingConfig
{
    ref array<string> whitelistedObjects;
    
    void TraderXVehicleParkingConfig()
    {
        whitelistedObjects = new array<string>;
        
        // Default whitelisted objects that don't block parking
        SetDefaultWhitelistedObjects();
    }
    
    private void SetDefaultWhitelistedObjects()
    {
        whitelistedObjects.Insert("Land_Grass");
        whitelistedObjects.Insert("Land_Stone");
        whitelistedObjects.Insert("Land_Dirt");
        whitelistedObjects.Insert("Land_Sand");
        whitelistedObjects.Insert("Land_Gravel");
        whitelistedObjects.Insert("ClutterCutter");
        whitelistedObjects.Insert("Particle");
        whitelistedObjects.Insert("EffectSound");
        whitelistedObjects.Insert("Land_Underground");
        whitelistedObjects.Insert("Terrain");
    }
    
    bool IsObjectWhitelisted(string objectName)
    {
        for (int i = 0; i < whitelistedObjects.Count(); i++)
        {
            if (objectName.Contains(whitelistedObjects[i]))
            {
                return true;
            }
        }
        return false;
    }
    
    void AddWhitelistedObject(string objectName)
    {
        if (whitelistedObjects.Find(objectName) == -1)
        {
            whitelistedObjects.Insert(objectName);
        }
    }
    
    void RemoveWhitelistedObject(string objectName)
    {
        int index = whitelistedObjects.Find(objectName);
        if (index != -1)
        {
            whitelistedObjects.Remove(index);
        }
    }
}
