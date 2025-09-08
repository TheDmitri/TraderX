class TraderXHelper
{
    static void CopyTraderXProductArray(array<ref TraderXProduct> src, out array<ref TraderXProduct> dst)
    {
        string data = TraderXJsonLoader<array<ref TraderXProduct>>.ObjectToString(src);
		TraderXJsonLoader<array<ref TraderXProduct>>.StringToObject(data, dst);
    }

    static PlayerBase GetPlayerByIdentity(PlayerIdentity identity)
    {
        int low = 0;
        int high = 0;
        GetGame().GetPlayerNetworkIDByIdentityID(identity.GetPlayerId(), low, high);
        return PlayerBase.Cast(GetGame().GetObjectByNetworkId(low, high));
    }
}