// TraderXGarage - NPC Service
// SINGLE CONFIG: the garage spawns AND tags its own attendant in BOTH modes, entirely from
// this mod's config (TXGSettings.npcs). No separate TraderX NPC entry is needed.
//
// TraderX present: each spawned NPC is set up via TraderX's own SetupTraderXNpc() (sets the
//   npcId so the garage action matches) but is NOT marked SetTraderNpc(), so TraderX's trade /
//   pulse actions (which gate on IsTraderNpc()) never fire on it — no menu collision.
// Standalone: basic placement + invulnerability; the action matches the NPC by className.

class TXGNpcService extends DayZularService
{
    private ref array<Object> m_Spawned = new array<Object>();

    void TXGNpcService()
    {
        GetDayZularEvents().OnMissionStart.Insert(OnMissionStart);
    }

    void ~TXGNpcService()
    {
        GetDayZularEvents().OnMissionStart.Remove(OnMissionStart);
    }

    protected void OnMissionStart()
    {
        if (!GetGame().IsServer())
            return;

#ifdef TraderX
        LogDiagnostics(true);
#else
        LogDiagnostics(false);
#endif

        SpawnNpcs();
    }

    // Prints the active mode + per-garage summary + validation warnings. Single source of truth.
    protected void LogDiagnostics(bool traderXMode)
    {
        TXGSettings settings = GetTXGConfigService().GetServerSettings();
        if (!settings)
            return;

        GetTXGLog().LogInfo("========================= TraderXGarage =========================");
        if (traderXMode)
            GetTXGLog().LogInfo("MODE: TraderX detected. Garage spawns + tags its own attendants from this config.");
        else
            GetTXGLog().LogInfo("MODE: standalone. Garage spawns its own attendants from this config.");
        GetTXGLog().LogInfo("Everything is configured HERE: " + TXGConstants.CONFIG_FILE);
        GetTXGLog().LogInfo("Garages configured: " + settings.npcs.Count().ToString());

        foreach (TXGNpcEntry npc : settings.npcs)
        {
            if (!npc)
                continue;

            GetTXGLog().LogInfo("  - garage '" + npc.name + "': npcId=" + npc.npcId.ToString() + " class=" + npc.className + " pos=" + npc.position.ToString() + " spots=" + npc.parkSpots.Count().ToString());

            // Validation warnings — common misconfigurations.
            if (npc.npcId < 0 || npc.npcId > 127)
                GetTXGLog().LogWarning("      ! npcId " + npc.npcId.ToString() + " out of range 0..127 — interaction will not match.");
            if (npc.className == "")
                GetTXGLog().LogWarning("      ! empty className — cannot spawn this NPC.");
            if (!npc.parkSpots || npc.parkSpots.Count() == 0)
                GetTXGLog().LogWarning("      ! no parkSpots — park-out will fail for this garage.");
        }
        GetTXGLog().LogInfo("================================================================");
    }

    protected void SpawnNpcs()
    {
        if (!GetGame().IsServer())
            return;

        TXGSettings settings = GetTXGConfigService().GetServerSettings();
        if (!settings || settings.npcs.Count() == 0)
        {
            GetTXGLog().LogInfo("[TXG] No garage NPCs configured");
            return;
        }

        Cleanup();

        foreach (TXGNpcEntry entry : settings.npcs)
        {
            if (!entry || entry.className == "")
                continue;

            Object obj = GetGame().CreateObject(entry.className, entry.position, false, false);
            if (!obj)
            {
                GetTXGLog().LogError("[TXG] Failed to spawn garage NPC: " + entry.className);
                continue;
            }

            SetupNpc(obj, entry);
            m_Spawned.Insert(obj);
        }

        GetTXGLog().LogInfo("[TXG] Spawned " + m_Spawned.Count() + " garage NPC(s)");
    }

    // Tag + stabilize the spawned NPC. In TraderX mode we reuse TraderX's own NPC setup so the
    // npcId syncs to clients exactly like a real trader (minus the trader bit).
    protected void SetupNpc(Object obj, TXGNpcEntry entry)
    {
#ifdef TraderX
        TraderXNpc tn = BuildTraderXNpc(entry);

        PlayerBase npcPlayer = PlayerBase.Cast(obj);
        if (npcPlayer)
        {
            npcPlayer.SetupTraderXNpc(tn);   // sets npcId/pos/ori/no-damage/loadout — NOT trader bit
            return;
        }

        BuildingBase npcBuilding = BuildingBase.Cast(obj);
        if (npcBuilding)
        {
            npcBuilding.SetupTraderXNpc(tn);
            return;
        }
#endif

        // Standalone, or a non-NPC object: basic placement + invulnerability.
        obj.SetPosition(entry.position);
        obj.SetOrientation(entry.orientation);
        EntityAI ent = EntityAI.Cast(obj);
        if (ent)
            ent.SetAllowDamage(false);
    }

#ifdef TraderX
    protected TraderXNpc BuildTraderXNpc(TXGNpcEntry entry)
    {
        array<ref TraderXLoadoutItem> loadouts = new array<ref TraderXLoadoutItem>();
        array<string> categories = new array<string>();
        array<string> currencies = new array<string>();

        TraderXNpc tn = new TraderXNpc(0, entry.className, entry.name, "Garage", entry.position, entry.orientation, loadouts, categories, currencies);
        tn.npcId = entry.npcId;   // constructor auto-generates; override with our configured id
        return tn;
    }
#endif

    protected void Cleanup()
    {
        foreach (Object o : m_Spawned)
        {
            if (o)
                GetGame().ObjectDelete(o);
        }
        m_Spawned.Clear();
    }
}
