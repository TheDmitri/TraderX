class TraderXSettingsRepository
{
    static void Save(TraderXGeneralSettings settings)
	{
		TraderXJsonLoader<TraderXGeneralSettings>.SaveToFile(TRADERX_CONFIG_FILE, settings);
	}

	static void CheckVersion(TraderXGeneralSettings settings)
	{
		bool updated = false;
		if(settings.version != TRADERX_CURRENT_VERSION)
		{
			settings.version = TRADERX_CURRENT_VERSION;
			updated = true;
		}

		if (settings.serverID == string.Empty)
		{
			settings.GenerateServerID(settings.serverID);
			updated = true;
		}

		if (MoveTradersOffReservedNpcIds(settings))
			updated = true;

		if (updated)
		{
			Save(settings);
		}
	}

	// npcId 125 opens TraderXGarage and 126 opens TraderXBanking (TXGConstants.GARAGE_NPC_ID,
	// TXBConstants.BANK_ID). A trading NPC that used one of them in an older config would open the
	// garage/bank instead of its shop, so it is moved to a free id (its parking file follows).
	static bool IsReservedNpcId(int npcId)
	{
		return npcId == 125 || npcId == 126;
	}

	static bool MoveTradersOffReservedNpcIds(TraderXGeneralSettings settings)
	{
		bool moved = false;
		foreach (TraderXNpc npc : settings.traders)
		{
			if (!npc || !IsReservedNpcId(npc.npcId) || !npc.categoriesId || npc.categoriesId.Count() == 0)
				continue;

			int newId = FindFreeNpcId(settings);
			if (newId < 0)
			{
				GetTraderXLogger().LogError("[TraderX] Trader '" + npc.givenName + "' uses reserved npcId " + npc.npcId + " and no free id is left - it will open the garage/bank instead of its shop");
				continue;
			}

			MoveParkingFile(npc.npcId, newId);
			GetTraderXLogger().LogWarning("[TraderX] Trader '" + npc.givenName + "' used reserved npcId " + npc.npcId + " (garage/bank) - moved to npcId " + newId);
			npc.npcId = newId;
			moved = true;
		}

		if (moved)
			TraderXVehicleParkingRepository.LoadAllParkingCollections();

		return moved;
	}

	static int FindFreeNpcId(TraderXGeneralSettings settings)
	{
		for (int id = 1; id < 125; id++)
		{
			bool used = false;
			foreach (TraderXNpc npc : settings.traders)
			{
				if (npc && npc.npcId == id)
				{
					used = true;
					break;
				}
			}
			if (!used)
				return id;
		}
		return -1;
	}

	static void MoveParkingFile(int oldId, int newId)
	{
		string oldFile = string.Format(TRADERX_VEHICLE_PARKING_FILE, oldId);
		string newFile = string.Format(TRADERX_VEHICLE_PARKING_FILE, newId);
		if (!FileExist(oldFile) || FileExist(newFile))
			return;

		CopyFile(oldFile, newFile);
		if (FileExist(newFile))
			DeleteFile(oldFile);
	}

	static void MakeDirectoriesIfNotExist()
	{
		string directories[] = {
			TRADERX_CONFIG_ROOT_SERVER,
			TRADERX_CONFIG_DIR_SERVER,
			TRADERX_PRODUCTS_DIR,
			TRADERX_CATEGORIES_DIR,
			TRADERX_DB_DIR_SERVER,
			TRADERX_FAVORITES,
			TRADERX_PRESETS,
			TRADERX_PLAYER_LICENSES_DIR,
			TRADERX_LOG_FOLDER,
			TRADERX_LOGGER_CONFIG_DIR,
			TRADERX_LOGGER_LOG_DIR
		};

		foreach(string dir: directories)
		{
			if (!FileExist(dir))
			{
				MakeDirectory(dir);
			}
		}
	}

	static ref TraderXGeneralSettings Load()
    {
        TraderXGeneralSettings settings = new TraderXGeneralSettings();

        MakeDirectoriesIfNotExist();

        if (FileExist(TRADERX_CONFIG_FILE))
        {
            GetTraderXLogger().LogInfo("Loading TraderX settings...");
            JsonFileLoader<TraderXGeneralSettings>.JsonLoadFile(TRADERX_CONFIG_FILE, settings);
			CheckVersion(settings);
			return settings;
        }

		settings.DefaultTraderXGeneralSettings();
        return settings;
    }
}