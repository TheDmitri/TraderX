// TraderXBanking - Constants
// Shared constants used across all script modules
// Wrapped in a class to avoid multiple declaration across script modules

class TXBConstants
{
    static const string RPC_NAMESPACE = "TraderXBanking";

    // All persistence lives under the shared TraderX profile root (mirrors TraderXCarLock/TraderXGarage).
    static const string ROOT_DIR = "$profile:TraderX/";
    static const string CONFIG_DIR = "$profile:TraderX/TraderXBanking/";
    static const string CONFIG_FILE = "$profile:TraderX/TraderXBanking/TraderXBankingConfig.json";
    static const string DB_DIR = "$profile:TraderX/TraderXBanking/Accounts/";
    static const string LEDGER_DIR = "$profile:TraderX/TraderXBanking/Ledgers/";
    static const string CONTACTS_DIR = "$profile:TraderX/TraderXBanking/Contacts/";
    static const string VAULT_DIR = "$profile:TraderX/TraderXBanking/Vaults/";
    static const string LOG_DIR = "$profile:TraderX/TraderXBanking/Logs/";
    static const string LOG_FILE = "$profile:TraderX/TraderXBanking/Logs/TXB_%1.log";

    static const string VAULT_VERSION = "1.0";
    static const int VAULT_DEFAULT_CAPACITY = 50;
    static const int VAULT_DEFAULT_FEE_PER_DAY = 0;

    static const int TRANSACTION_DEPOSIT = 0;
    static const int TRANSACTION_WITHDRAW = 1;
    static const int TRANSACTION_TRANSFER = 2;
    static const int TRANSACTION_TRANSFER_IN = 3;   // result-notification only (recipient side)

    static const float QUEUE_INTERVAL = 1.0;

    static const int BANK_ID = 126;  // 0x7E = 126 (7-bit max for NPC ID, was -2 but bit mask converts it)

    static const string CONFIG_VERSION = "1.0";
    static const string ACCOUNT_VERSION = "1.0";
    static const string LEDGER_VERSION = "1.0";
    static const string CONTACTS_VERSION = "1.0";

    // Ledger write buffering - flush on entry-count OR time-interval, whichever first.
    // Entries kept per player are capped at LEDGER_MAX_ENTRIES (trim on append).
    static const int LEDGER_FLUSH_THRESHOLD = 10;
    static const float LEDGER_FLUSH_INTERVAL = 30.0;
    static const int LEDGER_MAX_ENTRIES = 500;
}
