// TraderXGarage - Constants
// Wrapped in a class to avoid multiple declaration across script modules.

class TXGConstants
{
    static const string RPC_NAMESPACE = "TraderXGarage";
    static const string STORAGE_MOD   = "TraderXGarage";

    // Own subfolder under the shared TraderX profile root (was scattered in TraderX/ root).
    static const string ROOT_DIR    = "$profile:TraderX/";
    static const string CONFIG_DIR  = "$profile:TraderX/TraderXGarage/";
    static const string CONFIG_FILE = "$profile:TraderX/TraderXGarage/TraderXGarageConfig.json";
    static const string DB_DIR      = "$profile:TraderX/TraderXGarage/Database/";
    static const string INSURANCE_DIR = "$profile:TraderX/TraderXGarage/Insurance/";
    static const string PLAYER_INDEX_FILE = "$profile:TraderX/TraderXGarage/Database/PlayerIndex.json";
    static const string DEAD_CLASSES_FILE = "$profile:TraderX/TraderXGarage/DeadClasses.txt";
    static const string LOG_DIR     = "$profile:TraderX/TraderXGarage/Logs/";
    static const string LOG_FILE    = "$profile:TraderX/TraderXGarage/Logs/TXG_%1.log";

    static const string CONFIG_VERSION = "2.0";
    static const string DATA_VERSION   = "2.0";

    // NPC id assigned (via TraderX NPC system) to the garage attendant.
    // 7-bit value (0..127). Banking uses 126 — garage uses 125 to stay distinct.
    static const int GARAGE_NPC_ID = 125;

    // Max distance (m) between the player and a garage attendant for any garage request.
    static const float GARAGE_INTERACT_DISTANCE = 12.0;

    // vehicleId of the "vehicle next to you" row in the garage list (not a stored vehicle).
    static const string NEARBY_VEHICLE_ID = "__nearby__";

    // Ledger labels consumed by the banking payment provider.
    static const string FEE_LABEL_IN        = "GARAGE_FEE_IN";
    static const string FEE_LABEL_OUT       = "GARAGE_FEE_OUT";
    static const string FEE_LABEL_REPAIR    = "GARAGE_REPAIR";
    static const string FEE_LABEL_INSURANCE = "GARAGE_INSURANCE";

    // Vehicle hold reasons.
    static const string HOLD_NONE      = "";
    static const string HOLD_INSURANCE = "INSURANCE";
    static const string HOLD_IMPOUND   = "IMPOUND";

    // CF ModStorage vehicle stamp states.
    static const int STAMP_NONE    = 0;
    static const int STAMP_INSURED = 1;
    static const int STAMP_CLAIMED = 2;

    // Insurance policy statuses.
    static const int POLICY_ACTIVE  = 0;
    static const int POLICY_EXPIRED = 1;
    static const int POLICY_CLAIMED = 2;
    static const int POLICY_VOID    = 3;

    // Vehicle health levels (mirror TraderX item-state ordering).
    static const int HEALTH_PRISTINE      = 0;
    static const int HEALTH_WORN          = 1;
    static const int HEALTH_DAMAGED       = 2;
    static const int HEALTH_BADLY_DAMAGED = 3;
    static const int HEALTH_RUINED        = 4;
}
