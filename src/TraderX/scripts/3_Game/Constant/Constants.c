// Global root paths
const string TRADERX_CONFIG_ROOT_SERVER = "$profile:TraderX\\";

// TraderXConfig
const string TRADERX_CONFIG_DIR_SERVER = TRADERX_CONFIG_ROOT_SERVER + "TraderXConfig\\";

// Database
const string TRADERX_DB_DIR_SERVER = TRADERX_CONFIG_ROOT_SERVER + "TraderXDatabase\\";

// General settings
const string TRADERX_CONFIG_FILE = TRADERX_CONFIG_DIR_SERVER + "TraderXGeneralSettings.json";

// Currency
const string TRADERX_CURRENCY_CONFIG_FILE = TRADERX_CONFIG_DIR_SERVER + "TraderXCurrencySettings.json";

// Category
const string TRADERX_CATEGORIES_DIR = TRADERX_CONFIG_DIR_SERVER + "Categories\\";
const string TRADERX_CATEGORY_FILE = TRADERX_CATEGORIES_DIR + "%1.json";  // %1 = categoryId

// Products
const string TRADERX_PRODUCTS_DIR = TRADERX_CONFIG_DIR_SERVER + "Products\\";
const string TRADERX_PRODUCT_FILE = TRADERX_PRODUCTS_DIR + "%1.json";  // %1 = productId


// Stock
const string TRADERX_STOCK_DIR = TRADERX_DB_DIR_SERVER + "Stock\\";
const string TRADERX_STOCK_FILE = TRADERX_STOCK_DIR + "%1.json";  // %1 = stockId


// Player Licenses
const string TRADERX_PLAYER_LICENSES_DIR = TRADERX_DB_DIR_SERVER + "PlayerLicenses\\";
const string TRADERX_PLAYER_LICENSES_FILE = TRADERX_PLAYER_LICENSES_DIR + "playerLicense_%1.json";  // %1 = playerId

// Preset domain constants
const string TRADERX_PRESETS = TRADERX_CONFIG_ROOT_SERVER + "TraderXPresets\\";
const string TRADERX_PRESETS_FILE = TRADERX_PRESETS + "presets_%1.json";  // %1 = presetId

// Dynamic translation
const string TRADERX_DYNAMIC_TRANSLATION_CONFIG_FILE = TRADERX_CONFIG_DIR_SERVER + "TraderXDynamicTranslationSettings.json";

// Debug test cases
const string TRADERX_DEBUG_TEST_CASES_DIR = TRADERX_CONFIG_DIR_SERVER + "DebugTestCases\\";
const string TRADERX_DEBUG_TEST_CASE_FILE = TRADERX_DEBUG_TEST_CASES_DIR + "%1.json";  // %1 = testCaseId

// Debug settings
const string TRADERX_DEBUG_SETTINGS_FILE = TRADERX_CONFIG_DIR_SERVER + "TraderXDebugSettings.json";

// Logger - MUST be defined before dependent constants
const string TRADERX_LOG_FOLDER = TRADERX_CONFIG_ROOT_SERVER + "TraderXLogs\\";
const string TRADERX_LOGGER_CONFIG_DIR = TRADERX_LOG_FOLDER + "Config\\";
const string TRADERX_LOGGER_CONFIG_FILE = TRADERX_LOGGER_CONFIG_DIR + "LoggingSettings.json";
const string TRADERX_LOGGER_LOG_DIR = TRADERX_LOG_FOLDER + "Logs\\";
const string TRADERX_LOGGER_LOG_FILE = TRADERX_LOGGER_LOG_DIR + "TraderX_%1.log";

// CSV Configuration System
const string TRADERX_CSV_SOURCE_DIR = TRADERX_CONFIG_DIR_SERVER + "Source\\";
const string TRADERX_COMPILED_DIR = TRADERX_CONFIG_DIR_SERVER + "Compiled\\";
const string TRADERX_BACKUP_DIR = TRADERX_CONFIG_DIR_SERVER + "Backup\\";

// Compiled configuration files
const string TRADERX_COMPILED_PRODUCTS_FILE = TRADERX_COMPILED_DIR + "Products_compiled.json";
const string TRADERX_COMPILED_CATEGORIES_FILE = TRADERX_COMPILED_DIR + "Categories_compiled.json";

// Backup files
const string TRADERX_COMPILED_PRODUCTS_BACKUP = TRADERX_BACKUP_DIR + "Products_compiled.json.backup";
const string TRADERX_COMPILED_CATEGORIES_BACKUP = TRADERX_BACKUP_DIR + "Categories_compiled.json.backup";

// Configuration reports (depends on TRADERX_LOGGER_CONFIG_DIR defined above)
const string TRADERX_CONFIG_REPORT_FILE = TRADERX_LOGGER_CONFIG_DIR + "ConfigReport.log";

// Version
const string TRADERX_CURRENT_VERSION = "1.0.1";
