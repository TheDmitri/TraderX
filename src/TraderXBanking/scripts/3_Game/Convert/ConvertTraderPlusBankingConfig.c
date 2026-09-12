// TraderPlus V1 -> TraderXBanking config migration.
// Self-contained inside the TraderXBanking PBO (no TraderX-core class refs) so it also works
// in standalone mode where the core TraderX mod is not loaded.
//
// Runs once, before TXBSettings.Load(): if no TraderXBanking config exists yet AND a TraderPlus
// banking config is found, convert it, write TraderXBankingConfig.json, then archive the source.
//
// Note: TraderPlus banking config carries no currency denomination table (only a list of accepted
// class names without values), so currencies/bankNpcs are left at TraderXBanking defaults.

class oldTraderPlusBankingConfig
{
    string Version = "";
    int    BankingLogs;
    int    IsCreditCarNeededForTransaction;
    float  TransactionFees;
    int    DefaultStartCurrency;
    int    DefaultMaxCurrency;
    ref array<string> CurrenciesAccepted;

    void oldTraderPlusBankingConfig()
    {
        CurrenciesAccepted = new array<string>();
    }

    static string ResolveSource()
    {
        string candidate;

        candidate = TXBConstants.CONFIG_DIR + "TraderPlusBankingConfig.json";
        if (FileExist(candidate))
            return candidate;

        candidate = "$profile:TraderPlus/TraderPlusConfig/TraderPlusBankingConfig.json";
        if (FileExist(candidate))
            return candidate;

        return string.Empty;
    }

    static void ArchiveSource(string sourcePath)
    {
        if (sourcePath == string.Empty || !FileExist(sourcePath))
            return;

        string deletedDir = TXBConstants.CONFIG_DIR + "deleted/";
        if (!FileExist(deletedDir))
            MakeDirectory(deletedDir);

        string dest = deletedDir + "TraderPlusBankingConfig.json";
        CopyFile(sourcePath, dest);
        DeleteFile(sourcePath);
        GetTXBLog().LogInfo("[TXB Migration] Archived consumed legacy banking config: " + sourcePath + " -> " + dest);
    }

    void ConvertAndSave()
    {
        TXBSettings settings = new TXBSettings();

        settings.enableLogs            = (BankingLogs != 0);
        settings.requireCreditCard     = (IsCreditCarNeededForTransaction != 0);
        settings.transactionFeePercent = TransactionFees;
        settings.defaultStartBalance   = DefaultStartCurrency;
        settings.defaultMaxBalance     = DefaultMaxCurrency;
        // currencies + bankNpcs intentionally kept at TraderXBanking defaults (see header note).

        settings.Save();
        GetTXBLog().LogInfo("[TXB Migration] Converted TraderPlus banking config: fee=" + settings.transactionFeePercent + " creditCard=" + settings.requireCreditCard + " startBal=" + settings.defaultStartBalance + " maxBal=" + settings.defaultMaxBalance);
    }

    static void ConvertIfPresent()
    {
        // Never clobber an existing TraderXBanking config.
        if (FileExist(TXBConstants.CONFIG_FILE))
            return;

        string path = ResolveSource();
        if (path == string.Empty)
            return;

        GetTXBLog().LogInfo("[TXB Migration] Loading legacy banking config: " + path);
        oldTraderPlusBankingConfig old = new oldTraderPlusBankingConfig();
        JsonFileLoader<oldTraderPlusBankingConfig>.JsonLoadFile(path, old);
        old.ConvertAndSave();
        ArchiveSource(path);
    }
}
