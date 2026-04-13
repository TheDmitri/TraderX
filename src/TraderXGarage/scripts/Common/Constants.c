// TraderXGarage - Constants
// File path constants for server-side config and database storage

const string TXG_CONFIG_ROOT_SERVER     = "$profile:TraderXGarage\\";
const string TXG_LOGGER_DIR_SERVER      = TXG_CONFIG_ROOT_SERVER + "Logs\\";
const string TXG_CONFIG_DIR_SERVER      = TXG_CONFIG_ROOT_SERVER + "Config\\";
const string TXG_DB_DIR_SERVER          = TXG_CONFIG_ROOT_SERVER + "Database\\";
const string TXG_CONFIG_FILE            = TXG_CONFIG_DIR_SERVER + "GarageConfig.json";
const string TXG_DATABASE_PREFIX        = TXG_DB_DIR_SERVER + "Garage_";

// RPC namespace
const string TXG_RPC_NAMESPACE          = "TraderXGarage";

// RPC endpoints
const string TXG_RPC_GARAGE_REQUEST     = "GarageRequest";
const string TXG_RPC_GARAGE_RESPONSE    = "GarageResponse";
const string TXG_RPC_PARK_IN            = "ParkIn";
const string TXG_RPC_PARK_OUT           = "ParkOut";
const string TXG_RPC_SYNC_CONFIG        = "SyncConfig";

// NPC trader ID used for garage interaction
const int TXG_GARAGE_NPC_ID             = -3;

// Inventory location type flags for vehicle cargo serialization
const int TXG_VEHICLE_ATTACHMENT        = -1;
const int TXG_VEHICLE_CARGO             = -2;

// Rate limiting
const int TXG_RPC_RATE_LIMIT            = 5;
const int TXG_RPC_RATE_WINDOW           = 10;

// Transaction throttle interval (milliseconds)
const int TXG_TRANSACTION_INTERVAL_MS   = 2000;
