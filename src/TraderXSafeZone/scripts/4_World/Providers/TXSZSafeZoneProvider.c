/**
 * TXSZSafeZoneProvider — TraderXSafeZone implementation of ITraderXSafeZoneProvider
 * (US-033 cross-mod port). Server-side, in-process façade: delegates to the existing
 * SafeZone module/settings (no protection logic duplicated) and holds an in-memory
 * list of runtime "dynamic" zones for event mods (not persisted).
 *
 * Registered automatically when TraderXSafeZone loads (see SafeZoneModule), only
 * when TraderX is present (requires the ITraderXSafeZoneProvider interface).
 *
 * Mirrors the Banking provider pattern (TXBPaymentProvider).
 */
#ifdef TraderX
class TXSZSafeZoneProvider : ITraderXSafeZoneProvider
{
    // Runtime, non-persisted zones (e.g. temporary event zones). id >= base.
    protected ref array<ref TXSafeZoneLocation> m_DynamicZones;
    protected int m_NextDynamicId = 100000;   // high base — never collides with config ids

    void TXSZSafeZoneProvider()
    {
        m_DynamicZones = new array<ref TXSafeZoneLocation>();
    }

    protected TXSafeZoneSettings GetSettings()
    {
        TraderXSafeZoneModule mod = TraderXSafeZoneModule.Cast(CF_ModuleCoreManager.Get(TraderXSafeZoneModule));
        if (!mod)
            return null;
        return mod.settings;
    }

    override bool IsInSafeZone(Man player)
    {
        PlayerBase pb = PlayerBase.Cast(player);
        if (!pb)
            return false;

        // Authoritative distance check (config + dynamic zones); fall back to the
        // synced flag if settings are not available on this side.
        if (IsPositionInSafeZone(pb.GetPosition()))
            return true;

        return pb.IsInTXSafeZone();
    }

    override bool IsPositionInSafeZone(vector pos)
    {
        return FindZoneAt(pos) != null;
    }

    override string GetZoneNameAt(vector pos)
    {
        TXSafeZoneLocation loc = FindZoneAt(pos);
        if (!loc)
            return "";
        return loc.name;
    }

    override int RegisterDynamicZone(string name, vector pos, float radius)
    {
        if (radius <= 0)
            return -1;

        int id = m_NextDynamicId;
        m_NextDynamicId++;

        int radiusInt = (int)Math.Round(radius);
        TXSafeZoneLocation zone = new TXSafeZoneLocation(true, id, name, pos, radiusInt, false, new array<string>());
        m_DynamicZones.Insert(zone);
        return id;
    }

    override bool UnregisterDynamicZone(int zoneId)
    {
        for (int i = 0; i < m_DynamicZones.Count(); i++)
        {
            if (m_DynamicZones[i] && m_DynamicZones[i].id == zoneId)
            {
                m_DynamicZones.RemoveOrdered(i);
                return true;
            }
        }
        return false;
    }

    // Returns the first config or dynamic zone whose 2D radius contains pos, else null.
    protected TXSafeZoneLocation FindZoneAt(vector pos)
    {
        vector flatPos = Vector(pos[0], 0, pos[2]);

        TXSafeZoneSettings settings = GetSettings();
        if (settings && settings.locations)
        {
            foreach (TXSafeZoneLocation loc : settings.locations)
            {
                if (loc && loc.isActive && IsFlatWithin(flatPos, loc.position, loc.radius))
                    return loc;
            }
        }

        foreach (TXSafeZoneLocation dyn : m_DynamicZones)
        {
            if (dyn && IsFlatWithin(flatPos, dyn.position, dyn.radius))
                return dyn;
        }

        return null;
    }

    protected bool IsFlatWithin(vector flatPos, vector zonePos, float radius)
    {
        vector flatZone = Vector(zonePos[0], 0, zonePos[2]);
        return vector.Distance(flatPos, flatZone) <= radius;
    }
}
#endif // TraderX
