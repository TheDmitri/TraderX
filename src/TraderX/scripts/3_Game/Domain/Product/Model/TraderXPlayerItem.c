class TraderXPlayerItem
{
    string playerItemId;
    string className;
    int networkIdLow;
    int networkIdHigh;
    int depth;
    string productId;
    int healthLevel;

    static TraderXPlayerItem CreateAsPlayerItem(string classname, int idHigh, int idLow, int depthLevel, string prodId, int helthLevel)
    {
        TraderXPlayerItem tpItem = new TraderXPlayerItem();
        classname.ToLower();
        tpItem.className = classname;
        tpItem.networkIdHigh = idHigh;
        tpItem.networkIdLow = idLow;
        tpItem.depth = depthLevel;
        tpItem.productId = prodId;
        tpItem.playerItemId = TraderXProductId.GeneratePlayerItemId(classname, idHigh, idLow);
        tpItem.healthLevel = helthLevel;

        return tpItem;
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