class TraderXPlayerItem
{
    string playerItemId;
    string className;
    int networkIdLow;
    int networkIdHigh;
    int depth;
    string productId;
    int healthLevel;

    void TraderXPlayerItem(string classname = "", int idHigh = 0, int idLow = 0, int depthLevel = 0, string prodId = "", int helthLevel = 0)
    {
        if (classname != "")
        {
            classname.ToLower();
            this.className = classname;
            this.networkIdHigh = idHigh;
            this.networkIdLow = idLow;
            this.depth = depthLevel;
            this.productId = prodId;
            this.playerItemId = TraderXProductId.GeneratePlayerItemId(classname, idHigh, idLow);
            this.healthLevel = helthLevel;
        }
    }

    static TraderXPlayerItem CreateAsPlayerItem(string classname, int idHigh, int idLow, int depthLevel, string prodId, int helthLevel)
    {
        return new TraderXPlayerItem(classname, idHigh, idLow, depthLevel, prodId, helthLevel);
    }

    string ToPlayerItemString()
    {
        string result = "";
        result += "\n  playerItemId: " + playerItemId;
        result += "\n  className: " + className;
        result += "\n  networkId: " + networkIdHigh.ToString() + networkIdLow.ToString();
        result += "\n  depth: " + depth.ToString();
        result += "\n  productId: " + productId;
        return result;
    }

    string GetPlayerItemId()
    {
        return playerItemId;
    }

    string GetClassName()
    {
        return className;
    }

    int GetNetworkIdLow()
    {
        return networkIdLow;
    }

    int GetNetworkIdHigh()
    {
        return networkIdHigh;
    }

    int GetDepth()
    {
        return depth;
    }

    string GetProductId()
    {
        return productId;
    }

    void SetProductId(string productId)
    {
        this.productId = productId;
    }
}