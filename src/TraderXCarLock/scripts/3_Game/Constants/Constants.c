const string TX_CARLOCK_VERSION = "1.0";

const string TX_CARLOCK_CONFIG_ROOT = "$profile:TraderXCarLock\\";
const string TX_CARLOCK_CONFIG_FILENAME = TX_CARLOCK_CONFIG_ROOT + "TraderXCarLockSettings.json";
const string TX_CARLOCK_DB_DIR = TX_CARLOCK_CONFIG_ROOT + "Database\\";
const string TX_CARLOCK_DB_FILENAME = TX_CARLOCK_DB_DIR + "%1.json";

const string TX_CARLOCK_LOGGER_ROOT = TX_CARLOCK_CONFIG_ROOT + "Logging\\";
const string TX_CARLOCK_LOGGER_CONFIG_DIR = TX_CARLOCK_LOGGER_ROOT + "Config\\";
const string TX_CARLOCK_LOGGER_CONFIG_FILE = TX_CARLOCK_LOGGER_CONFIG_DIR + "CarLockLoggingSettings.json";

const string TX_CARLOCK_LOGGER_LOG_DIR = TX_CARLOCK_LOGGER_ROOT + "Logs\\";
const string TX_CARLOCK_LOGGER_LOG_FILE = TX_CARLOCK_LOGGER_LOG_DIR + "CarLock_%1.log";

const int TX_CL_UNDEFINED = -1;
const int TX_CL_PASSWORD_LENGTH = 8;

enum ETXCarLockSounds
{
    NO_SOUND = 0,
    LOCK,
    UNLOCK,
    ALARM
}
