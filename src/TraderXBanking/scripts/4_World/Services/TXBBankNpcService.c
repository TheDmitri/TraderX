// TraderXBanking - Bank Object Service
// Spawns static bank objects (buildings) when TraderX is not present
// (In TraderX mode, NPCs/Buildings are managed by TraderX's NPC system)

class TXBBankNpcService extends DayZularService
{
	private ref array<Object> m_SpawnedNpcs = new array<Object>();

	void TXBBankNpcService()
	{
		// Subscribe to mission start event
		GetDayZularEvents().OnMissionStart.Insert(OnMissionStart);
	}

	void ~TXBBankNpcService()
	{
		GetDayZularEvents().OnMissionStart.Remove(OnMissionStart);
	}

	protected void OnMissionStart()
	{
		// Only spawn NPCs in standalone mode (when TraderX is not present)
#ifdef TraderX
		GetTXBLog().LogInfo("[TXB] Bank NPC service: TraderX detected, using TraderX NPC system");
		return;
#else
		SpawnBankNpcs();
#endif
	}

	protected void SpawnBankNpcs()
	{
		if (!GetGame().IsServer())
			return;

		TXBSettings settings = GetTXBConfigService().GetServerSettings();
		if (!settings)
			return;

		if (settings.bankNpcs.Count() == 0)
		{
			GetTXBLog().LogInfo("[TXB] No bank objects configured (standalone mode)");
			return;
		}

		// Cleanup previously spawned objects (for reloads)
		CleanupSpawnedNpcs();

		foreach (TXBBankNpcEntry entry : settings.bankNpcs)
		{
			SpawnBankNpc(entry);
		}

		GetTXBLog().LogInfo("[TXB] Spawned " + m_SpawnedNpcs.Count() + " bank objects");
	}

	protected void SpawnBankNpc(TXBBankNpcEntry entry)
	{
		if (entry.className == string.Empty)
			return;

		// Spawn the bank object (must extend BuildingBase)
		Object bankObj = GetGame().CreateObject(entry.className, entry.position, false, true, true);
		if (!bankObj)
		{
			GetTXBLog().LogError("[TXB] Failed to spawn bank object: " + entry.className);
			return;
		}

		// Setup as bank - only BuildingBase is supported in standalone mode.
		// SetupTXBBankNpc only exists when TraderX is absent (modded BuildingBase is #ifndef TraderX),
		// so guard the call at compile time - this whole service no-ops when TraderX is present anyway.
#ifndef TraderX
		BuildingBase building = BuildingBase.Cast(bankObj);
		if (building)
		{
			building.SetupTXBBankNpc(entry.position, entry.orientation);
		}
		else
		{
			GetTXBLog().LogWarning("[TXB] Spawned object is not a BuildingBase (may not work as bank): " + entry.className);
		}
#endif

		m_SpawnedNpcs.Insert(bankObj);

		string name = entry.name;
		if (name == string.Empty)
			name = entry.className;

		GetTXBLog().LogInfo("[TXB] Spawned bank object: " + name + " at " + entry.position.ToString());
	}

	protected void CleanupSpawnedNpcs()
	{
		foreach (Object npc : m_SpawnedNpcs)
		{
			if (npc)
				GetGame().ObjectDelete(npc);
		}
		m_SpawnedNpcs.Clear();
	}

	void ReloadBankNpcs()
	{
#ifdef TraderX
		// Don't reload in TraderX mode - TraderX manages NPCs
		return;
#else
		GetTXBLog().LogInfo("[TXB] Reloading bank NPCs...");
		SpawnBankNpcs();
#endif
	}
}

// Global service getter
static ref TXBBankNpcService g_TXBBankNpcService;

static TXBBankNpcService GetTXBBankNpcService()
{
	if (!g_TXBBankNpcService)
		g_TXBBankNpcService = new TXBBankNpcService();
	return g_TXBBankNpcService;
}
