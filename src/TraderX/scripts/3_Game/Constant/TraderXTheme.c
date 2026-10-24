/**
 * TraderXTheme — the ONLY hardcoded colors left in TraderX code.
 *
 * Everything visual that can be a style lives in the atlas (source:
 * datasets/gui/_fixtures/field-ledger.html → build-styles.sh). This class holds
 * only the colors that are applied at RUNTIME by state (a SetColor tint that no
 * static style can express): item-card warning tint, nav/slot active state,
 * tooltip availability, stock-low, preset marker.
 *
 * Re-skinning: a modder changes the atlas tokens for 99% of the UI; these few
 * runtime colors are the remaining 1%. Keep them aligned with the Nasdara palette
 * (ember #D9762F / cream #F2E9D8 / muted #A39276 / good #7FA65A / bad #E07A63).
 *
 * Replaces the old sprawling TraderXViewStyles (deleted in US-025); dead tokens
 * (UI_*, CATEGORY_*, TEXT_IDLE/GRAY) were dropped.
 */
class TraderXTheme
{
    // Item card content tint (multiplied onto the TXSurface/TXCard style):
    // IDLE is ~white so the style shows; WARNING is a light warm tint.
    static int CARD_IDLE    = 0xfffefefe;
    static int CARD_WARNING = 0xffe8b08a;

    // Tooltip variant/attachment availability text.
    static int TEXT_OK   = 0xff7fa65a;  // available (good)
    static int TEXT_WARN = 0xffe07a63;  // unavailable (bad)

    // Nav tab label.
    static int NAV_ACTIVE   = 0xfff2e9d8;  // cream (the tab underline is ember)
    static int NAV_INACTIVE = 0xffa39276;  // muted

    // Player slot icon — selected matches the nav tab gold/orange.
    static int SLOT_ACTIVE   = 0xffd9762f;  // ember
    static int SLOT_INACTIVE = 0xffa39276;  // muted

    // Item stock-low text.
    static int STOCK_LOW = 0xffe07a63;  // bad

    // Preset default marker.
    static int PRESET_DEFAULT = 0xffd9762f;
    static int PRESET_NORMAL  = 0xfffefefe;

    // Notification detail text (legacy dormant stack).
    static int NOTIF_SUCCESS = 0xff7fa65a;
    static int NOTIF_ERROR   = 0xffe07a63;
}
