// TraderXCarLock - constants, sentinel values and enums.
// Prefix TXCL* everywhere to avoid cross-PBO collisions (TL_CarLock / TraderPlusCarLock).

const string TXCL_VERSION = "1.0";

const string TXCL_PROFILE_ROOT     = "$profile:TraderX/";
const string TXCL_CONFIG_ROOT      = "$profile:TraderX/TraderXCarLock/";
const string TXCL_CONFIG_FILENAME  = TXCL_CONFIG_ROOT + "TXCLSettings.json";
const string TXCL_DB_DIR           = TXCL_CONFIG_ROOT + "Database/";
const string TXCL_LOG_DIR          = TXCL_CONFIG_ROOT + "Logs/";
const string TXCL_LOG_FILE         = TXCL_LOG_DIR + "TXCL_%1.log";
// CfgMods class carrying storageVersion; the CF ModStorage key for vehicle lock data.
const string TXCL_STORAGE_MOD      = "TraderXCarLock";
const string TXCL_DB_FILENAME      = TXCL_DB_DIR + "%1.json";

// Sentinel for "no owner / no password set".
const int TXCL_UNDEFINED = -1;

// CF RPC namespace for every client<->server message in this mod.
const string TXCL_RPC_NAMESPACE = "TXCarLock";

// Sound ids synced through the vehicle net-sync int (low byte = current sound).
enum ETXCLCarLockSounds
{
    NO_SOUND = 0,
    LOCK,
    UNLOCK,
    ALARM
}

// Vehicle families for per-type lockpick configuration.
enum ETXCLVehicleType
{
    UNKNOWN = 0,
    CAR,
    BOAT,
    HELI
}
