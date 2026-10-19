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

        // TraderPlus may still be loaded on this server: never touch its own config folder.
        // The TraderX-side config written by this migration already prevents a re-run.
        if (sourcePath.IndexOf("$profile:TraderPlus/") == 0)
        {
            GetTXBLog().LogInfo("[TXB Migration] Imported " + sourcePath + " (left in place)");
            return;
        }

        string deletedDir = TXBConstants.CONFIG_DIR + "deleted/";
        if (!FileExist(deletedDir))
            MakeDirectory(deletedDir);

        string dest = deletedDir + "TraderPlusBankingConfig.json";
        CopyFile(sourcePath, dest);
        if (!FileExist(dest))
        {
            GetTXBLog().LogError("[TXB Migration] Could not archive " + sourcePath + " to " + dest + " - keeping the original");
            return;
        }

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

        TXBTraderPlusCurrencyImport.Apply(settings.currencies, CurrenciesAccepted);
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

// TraderPlus currencies live in TraderPlusGeneralConfig.json: [{"ClassName": "a,b", "Value": n}].
class TXBTraderPlusCurrency
{
    string ClassName;
    int    Value;
}

class TXBTraderPlusCurrencyList
{
    ref array<ref TXBTraderPlusCurrency> Currencies;
}

class TXBTraderPlusCurrencyImport
{
    static const string GENERAL_CONFIG = "$profile:TraderPlus/TraderPlusConfig/TraderPlusGeneralConfig.json";

    // Replaces the default TraderX_Money_* list with the notes TraderPlus players actually hold.
    static void Apply(array<ref TXBCurrencyEntry> currencies, array<string> acceptedOnly)
    {
        if (!FileExist(GENERAL_CONFIG))
            return;

        TXBTraderPlusCurrencyList list = new TXBTraderPlusCurrencyList();
        string error;
        if (!JsonFileLoader<TXBTraderPlusCurrencyList>.LoadFile(GENERAL_CONFIG, list, error) || !list.Currencies || list.Currencies.Count() == 0)
            return;

        currencies.Clear();
        foreach (TXBTraderPlusCurrency currency : list.Currencies)
        {
            TStringArray classNames = new TStringArray();
            currency.ClassName.Split(",", classNames);
            foreach (string className : classNames)
            {
                className.TrimInPlace();
                if (className == string.Empty)
                    continue;
                if (acceptedOnly && acceptedOnly.Count() > 0 && acceptedOnly.Find(className) == -1)
                    continue;
                currencies.Insert(new TXBCurrencyEntry(className, currency.Value));
            }
        }
        GetTXBLog().LogInfo("[TXB Migration] Imported " + currencies.Count() + " TraderPlus currency class(es)");
    }
}
