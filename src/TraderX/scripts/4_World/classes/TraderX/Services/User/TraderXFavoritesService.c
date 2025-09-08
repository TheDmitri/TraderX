class TraderXFavoritesService
{
    static ref TraderXFavoritesService m_instance;

    static ref ScriptInvoker Event_OnFavoriteChangedCallBack = new ScriptInvoker();

    ref set<UUID> m_favorites;
    string filePath;

    void TraderXFavoritesService()
    {
        string serverID = GetTraderXModule().generalSettings.serverID;
        filePath = string.Format(TRADERX_FAVORITES_FILE, serverID);
        m_favorites = new set<UUID>();
        LoadTraderXProductsFavorites();
    }

    static TraderXFavoritesService GetInstance()
    {
        if (!m_instance)
        {
            m_instance = new TraderXFavoritesService();
        }
        return m_instance;
    }

    bool IsFavorite(TraderXProduct item)
    {
        return m_favorites.Find(item.productId) != -1;
    }

    void AddFavorite(TraderXProduct item)
    {
        if (item)
        {
            m_favorites.Insert(item.productId);
            Event_OnFavoriteChangedCallBack.Invoke(this);
            StoreTraderXProductsFavorites();
        }
    }

    void RemoveFavorite(TraderXProduct item)
    {
        if (item)
        {
            m_favorites.RemoveItem(item.productId);
            Event_OnFavoriteChangedCallBack.Invoke(this);
            StoreTraderXProductsFavorites();
        }
    }

    void StoreTraderXProductsFavorites()
    {
        string errorMessage;
        JsonFileLoader<set<UUID>>.SaveFile(filePath, m_favorites, errorMessage);
    }

    void LoadTraderXProductsFavorites()
    {
        if (!FileExist(TRADERX_CONFIG_ROOT_SERVER))
        {
            MakeDirectory(TRADERX_CONFIG_ROOT_SERVER);
        }

        if (!FileExist(TRADERX_FAVORITES))
        {
            MakeDirectory(TRADERX_FAVORITES);
        }

        if (FileExist(filePath))
        {
            string errorMessage;
            JsonFileLoader<set<UUID>>.LoadFile(filePath, m_favorites, errorMessage);
            return;
        }

        StoreTraderXProductsFavorites();
    }
}