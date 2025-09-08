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

		if (updated)
		{
			Save(settings);
		}
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