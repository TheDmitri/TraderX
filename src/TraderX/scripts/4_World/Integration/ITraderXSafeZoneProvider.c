/**
 * Port: safezone provider. Server-side, in-process calls.
 * Vanilla types only.
 */
class ITraderXSafeZoneProvider
{
    bool   IsInSafeZone(Man player) { return false; }
    bool   IsPositionInSafeZone(vector pos) { return false; }
    string GetZoneNameAt(vector pos) { return ""; }
    // Dynamic runtime zone (e.g. event mod). Returns a zone id (>0) or -1.
    int    RegisterDynamicZone(string name, vector pos, float radius) { return -1; }
    bool   UnregisterDynamicZone(int zoneId) { return false; }
}
