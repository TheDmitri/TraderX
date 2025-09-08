class TraderXLicenseRepository
{
    static void MakeDirectoryIFNotExist()
    {
        if(!FileExist(TRADERX_PLAYER_LICENSES_DIR))
        {
            MakeDirectory(TRADERX_PLAYER_LICENSES_DIR);
        }
    }

    static TraderXPlayerLicenses Load(string id, string name)
    {
        MakeDirectoryIFNotExist();

        TraderXPlayerLicenses playerLicenses = new TraderXPlayerLicenses(id, name);
        string filePath = string.Format(TRADERX_PLAYER_LICENSES_FILE, id);
        if(FileExist(filePath))
        {
            JsonFileLoader<TraderXPlayerLicenses>.JsonLoadFile(filePath, playerLicenses);
        }

        return playerLicenses;
    }
}