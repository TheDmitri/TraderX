// TraderXBanking - Constants
// Shared constants used across all script modules

const string TXB_RPC_NAMESPACE = "TraderXBanking";

// File paths (server profile relative)
const string TXB_CONFIG_DIR = "$profile:TraderXBanking/";
const string TXB_CONFIG_FILE = TXB_CONFIG_DIR + "TraderXBankingConfig.json";
const string TXB_DB_DIR = TXB_CONFIG_DIR + "Accounts/";

// Transaction types
const int TXB_TRANSACTION_DEPOSIT = 0;
const int TXB_TRANSACTION_WITHDRAW = 1;
const int TXB_TRANSACTION_TRANSFER = 2;

// Transaction queue processing interval (seconds)
const float TXB_QUEUE_INTERVAL = 1.0;

// Bank entity identifier (for NPC/building interaction)
const int TXB_BANK_ID = -2;

// Config version for data migration
const string TXB_CONFIG_VERSION = "1.0";
const string TXB_ACCOUNT_VERSION = "1.0";
