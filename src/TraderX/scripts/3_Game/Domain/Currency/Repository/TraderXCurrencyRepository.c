class TraderXCurrencyRepository
{
    static void Save(TraderXCurrencyTypeCollection collection)
	{
		JsonFileLoader<TraderXCurrencyTypeCollection>.JsonSaveFile(TRADERX_CURRENCY_CONFIG_FILE, collection);
	}

	static void CheckVersion(TraderXCurrencyTypeCollection collection)
	{
		if(collection.version != TRADERX_CURRENT_VERSION)
		{
			collection.version = TRADERX_CURRENT_VERSION;
			Save(collection);
		}
	}

	static void MakeDirectoriesIfNotExist()
	{
		if ( !FileExist( TRADERX_CONFIG_ROOT_SERVER ) )
			MakeDirectory( TRADERX_CONFIG_ROOT_SERVER );

		if ( !FileExist( TRADERX_CONFIG_DIR_SERVER ) )
			MakeDirectory( TRADERX_CONFIG_DIR_SERVER );
	}

    static ref TraderXCurrencyTypeCollection Load()	
	{
		TraderXCurrencyTypeCollection collection = new TraderXCurrencyTypeCollection();

		MakeDirectoriesIfNotExist();

		if (FileExist(TRADERX_CURRENCY_CONFIG_FILE)) {
			JsonFileLoader<TraderXCurrencyTypeCollection>.JsonLoadFile(TRADERX_CURRENCY_CONFIG_FILE, collection);
			CheckVersion(collection);
			return collection;
		}
		
		collection.DefaultCurrencyTypeCollection();
        Save(collection);
		return collection;
	}
}
